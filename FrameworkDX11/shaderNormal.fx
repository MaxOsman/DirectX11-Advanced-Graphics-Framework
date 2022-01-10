//--------------------------------------------------------------------------------------
// 
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

// the lighting equations in this code have been taken from https://www.3dgep.com/texturing-lighting-directx-11/
// with some modifications by David White

//--------------------------------------------------------------------------------------
// Constant Buffer Variables
//--------------------------------------------------------------------------------------
cbuffer ConstantBuffer : register( b0 )
{
	matrix World;
	matrix View;
	matrix Projection;
	float4 vOutputColor;
}

Texture2D txDiffuse : register(t0);
Texture2D txNormal : register(t1);
Texture2D txParallax : register(t2);
Texture2D txBloom : register(t3);

SamplerState samLinear : register(s0);

#define MAX_LIGHTS 1
// Light types.
#define DIRECTIONAL_LIGHT 0
#define POINT_LIGHT 1
#define SPOT_LIGHT 2

struct _Material
{
	float4  Emissive;       // 16 bytes
							//----------------------------------- (16 byte boundary)
	float4  Ambient;        // 16 bytes
							//------------------------------------(16 byte boundary)
	float4  Diffuse;        // 16 bytes
							//----------------------------------- (16 byte boundary)
	float4  Specular;       // 16 bytes
							//----------------------------------- (16 byte boundary)
	float   SpecularPower;  // 4 bytes
	int    UseTexture;		// 4 bytes
	float2  Padding;        // 8 bytes
							//----------------------------------- (16 byte boundary)
};  // Total:               // 80 bytes ( 5 * 16 )

cbuffer MaterialProperties : register(b1)
{
	_Material Material;
};

struct Light
{
	float4      Position;               // 16 bytes
										//----------------------------------- (16 byte boundary)
	float4      Direction;              // 16 bytes
										//----------------------------------- (16 byte boundary)
	float4      Color;                  // 16 bytes
										//----------------------------------- (16 byte boundary)
	float       SpotAngle;              // 4 bytes
	float       ConstantAttenuation;    // 4 bytes
	float       LinearAttenuation;      // 4 bytes
	float       QuadraticAttenuation;   // 4 bytes
										//----------------------------------- (16 byte boundary)
	int         LightType;              // 4 bytes
	bool        Enabled;                // 4 bytes
	int2        Padding;                // 8 bytes
										//----------------------------------- (16 byte boundary)
};  // Total:                           // 80 bytes (5 * 16)

cbuffer LightProperties : register(b2)
{
	float4 EyePosition;                 // 16 bytes
										//----------------------------------- (16 byte boundary)
	float4 GlobalAmbient;               // 16 bytes
										//----------------------------------- (16 byte boundary)
	Light Lights[MAX_LIGHTS];           // 80 * 8 = 640 bytes
};

cbuffer BillboardProperties : register(b3)
{
	float4 EyePos;
	float4 UpVector;
};

cbuffer BlurProperties : register(b4)
{
	int isHorizontal;
	float2 mouseChange;
	float Padding;
}

//--------------------------------------------------------------------------------------

struct VS_INPUT
{
	float4 Pos : POSITION;
	float3 Norm : NORMAL;
	float2 Tex : TEXCOORD0;
	float3 Tan : TANGENT;
	float3 Binorm : BINORMAL;
};

struct RTT_VS_INPUT
{
	float4 Pos : POSITION;
	float2 Tex : TEXCOORD0;
};

struct PS_INPUT
{
    float4 Pos : SV_POSITION;
	float4 worldPos : POSITION;
	float3 Norm : NORMAL;
	float2 Tex : TEXCOORD0;
	float3 eyeVectorTS : POSITION4;
	float3 lightVectorTS : POSITION5;
};

struct RTT_PS_INPUT
{
	float4 Pos : SV_POSITION;
	float2 Tex : TEXCOORD0;
};

float4 DoDiffuse(float4 lightColour, float3 L, float3 N)
{
	float NdotL = max(0, dot(N, L));
	return lightColour * NdotL;
}

float4 DoSpecular(float3 vertexToEye, float3 lightDirectionToVertex, float3 Normal)
{
	float3 lightDir = normalize(-lightDirectionToVertex);
	vertexToEye = normalize(vertexToEye);

	float lightIntensity = saturate(dot(Normal, lightDir));
	float4 specular = float4(0, 0, 0, 0);
	if (lightIntensity > 0.0f)
	{
		float3 reflection = normalize(2 * lightIntensity * Normal - lightDir);
		specular = pow(saturate(dot(reflection, vertexToEye)), Material.SpecularPower); // 32 = specular power
	}

	return specular;
}

float DoAttenuation(Light light, float d)
{
	return 1.0f / (light.ConstantAttenuation + light.LinearAttenuation * d + light.QuadraticAttenuation * d * d);
}

struct LightingResult
{
	float4 Diffuse;
	float4 Specular;
};

LightingResult DoPointLight(Light light, float3 vertexToEye, float3 N, float3 vertexToLight)
{
	LightingResult result;

	float3 LightDirectionToVertex = mul(vertexToLight, -1.0f);
	float distance = length(LightDirectionToVertex);
	LightDirectionToVertex = LightDirectionToVertex / distance;

	distance = length(vertexToLight);
	vertexToLight = vertexToLight / distance;

	float attenuation;
	if (light.LightType != DIRECTIONAL_LIGHT)
	{
		attenuation = DoAttenuation(light, distance);
	}
	else
	{
		attenuation = 1;
	}

	result.Diffuse = DoDiffuse(light.Color, vertexToLight, N) * attenuation;
	result.Specular = DoSpecular(vertexToEye, LightDirectionToVertex, N) * attenuation;

	return result;
}

LightingResult ComputeLighting(float3 N, float3 vertexToEye, float3 vertexToLight)
{
	LightingResult result = DoPointLight(Lights[0], vertexToEye, N, vertexToLight);

	return result;
}

float ParallaxSelfShadowing(float3 lightDir, float2 texCoords, float parallaxScale)
{
	/***********************************************
	MARKING SCHEME: Parallax Mapping
	DESCRIPTION: Self-shadowing parallax occlusion mapping
	***********************************************/
	const float initialHeight = 1.0f;
	float shadowMultiplier = 1.0f;

	// If surface pointing towards light
	if (dot(float3(0.0f, 0.0f, 1.0f), lightDir) > 0)
	{
		float numSamplesUnderSurface = 0;
		shadowMultiplier = 0;

		float numLayers = lerp(30, 15, abs(dot(float3(0.0f, 0.0f, 1.0f), lightDir)));
		float layerHeight = initialHeight / numLayers;
		float2 dTex = parallaxScale * lightDir.xy / lightDir.z / numLayers;

		// Initialise parameters
		float currentLayerHeight = initialHeight - layerHeight;

		texCoords += dTex;
		float height = txParallax.Sample(samLinear, texCoords).x;

		int stepIndex = 1;
		[unroll(32)]
		while (currentLayerHeight > 0)
		{
			if (height < currentLayerHeight)
			{
				numSamplesUnderSurface += 1;

				float newShadowMultiplier = (currentLayerHeight - height) * (1.0f - stepIndex / numLayers);
				shadowMultiplier = max(shadowMultiplier, newShadowMultiplier);
			}

			stepIndex += 1;
			currentLayerHeight -= layerHeight;

			texCoords += dTex;
			height = txParallax.Sample(samLinear, texCoords).x;
		}

		if (numSamplesUnderSurface < 1)
		{
			shadowMultiplier = 1.0f;
		}
		else
		{
			shadowMultiplier = 1.0f - shadowMultiplier;
		}
	}

	return (1.0f - shadowMultiplier);
}

//--------------------------------------------------------------------------------------
// Vertex Shaders
//--------------------------------------------------------------------------------------
VS_INPUT VS( VS_INPUT input )
{
	VS_INPUT output = (VS_INPUT)0;

	output = input;

	return output;
}

RTT_PS_INPUT RTT_VS( RTT_VS_INPUT input )
{
	RTT_PS_INPUT output = (RTT_PS_INPUT)0;

	output.Pos = input.Pos;
	output.Tex = input.Tex;

	return output;
}

//--------------------------------------------------------------------------------------
// Geometry Shaders
//--------------------------------------------------------------------------------------

[maxvertexcount(6)]
void GS(triangle VS_INPUT input[3], inout TriangleStream<PS_INPUT> OutputStream)
{
	PS_INPUT output = (PS_INPUT)0;
	for (int i = 0; i < 3; ++i)
	{
		output.Pos = mul(input[i].Pos, World);
		output.worldPos = output.Pos;
		output.Pos = mul(output.Pos, View);
		output.Pos = mul(output.Pos, Projection);

		output.Tex = input[i].Tex;

		// multiply the normal by the world transform (to go from model space to world space)
		output.Norm = mul(float4(input[i].Norm, 1), World).xyz;

		// Week 2 lecture slides
		float3 T = mul(float4(input[i].Tan, 1), World).xyz;
		float3 B = mul(float4(input[i].Binorm, 1), World).xyz;
		float3x3 TBN = float3x3(T, B, output.Norm);
		TBN = transpose(TBN);

		output.eyeVectorTS = normalize(mul((EyePosition - output.worldPos).xyz, TBN));
		output.lightVectorTS = mul((Lights[0].Position - output.worldPos).xyz, TBN);

		OutputStream.Append(output);
	}
}

// Based on www.braynzarsoft.net/viewtutorial/q16390-36-billboarding-geometry-shader
[maxvertexcount(4)]
void GS_BILL(point RTT_PS_INPUT input[1], inout TriangleStream<RTT_PS_INPUT> OutputStream)
{
	/***********************************************
	MARKING SCHEME: Advanced graphics techniques
	DESCRIPTION: Billboarded sprites
	***********************************************/
	float3 planeNormal = input[0].Pos.xyz - EyePos.xyz;
	planeNormal = normalize(planeNormal);

	float3 rightVector = normalize(cross(planeNormal, UpVector.xyz));
	rightVector /= 2;

	float3 vert[4];
	vert[0] = input[0].Pos.xyz - rightVector;
	vert[1] = input[0].Pos.xyz + rightVector;
	vert[2] = input[0].Pos.xyz - rightVector + UpVector.xyz;
	vert[3] = input[0].Pos.xyz + rightVector + UpVector.xyz;

	float2 texCoord[4];
	texCoord[0] = float2(0, 1);
	texCoord[1] = float2(1, 1);
	texCoord[2] = float2(0, 0);
	texCoord[3] = float2(1, 0);

	RTT_PS_INPUT output;
	for (int i = 0; i < 4; ++i)
	{
		//output.worldPos = float4(vert[i], 1.0f);
		output.Pos = mul(float4(vert[i], 1.0f), View);
		output.Pos = mul(output.Pos, Projection);

		output.Tex = texCoord[i];

		OutputStream.Append(output);
	}
}

[maxvertexcount(6)]
void GS_Depth(triangle RTT_PS_INPUT input[3], inout TriangleStream<RTT_PS_INPUT> OutputStream)
{
	/***********************************************
	MARKING SCHEME: Simple screen space effect
	DESCRIPTION: Depth buffer rendering
	***********************************************/
	RTT_PS_INPUT output = (RTT_PS_INPUT)0;
	for (int i = 0; i < 3; ++i)
	{
		input[i].Pos.w = 1.0f;
		output.Pos = mul(input[i].Pos, World);
		output.Pos = mul(output.Pos, View);
		output.Pos = mul(output.Pos, Projection);

		output.Tex = input[i].Tex;

		OutputStream.Append(output);
	}
}

//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------

float4 Standard_PS(PS_INPUT IN)
{
	float4 texNormal = float4(0.0f, 0.0f, 1.0f, 0.0f);
	float2 texCoords = IN.Tex;
	const float parallaxScale = 0.1f;
	if (Material.UseTexture > 0)
	{
		if (Material.UseTexture == 2)
		{
			/***********************************************
			MARKING SCHEME: Parallax Mapping
			DESCRIPTION: Standard parallax mapping
			***********************************************/
			float height = txParallax.Sample(samLinear, texCoords).x;
			float3 viewDir = IN.eyeVectorTS;

			float2 p = viewDir.xy / viewDir.z * height * parallaxScale;
			texCoords = texCoords - p;
		}
		else if (Material.UseTexture == 3 || Material.UseTexture == 4)
		{
			/***********************************************
			MARKING SCHEME: Parallax Mapping
			DESCRIPTION: Parallax occlusion mapping
			***********************************************/
			float height = txParallax.Sample(samLinear, texCoords).x;
			float3 viewDir = IN.eyeVectorTS;

			float numLayers = lerp(15, 10, abs(dot(float3(0.0f, 0.0f, 1.0f), viewDir)));
			float2 dTex = parallaxScale * viewDir.xy / viewDir.z / numLayers;

			float currentLayerHeight = 0.0f;
			[unroll(32)]
			while (height > currentLayerHeight)
			{
				currentLayerHeight += (1.0f / numLayers);
				texCoords -= dTex;

				height = txParallax.Sample(samLinear, texCoords).x;
			}

			float2 prevTexCoords = texCoords + dTex;
			float nextHeight = height - currentLayerHeight;
			float prevHeight = txParallax.Sample(samLinear, texCoords).x - currentLayerHeight + (1.0f / numLayers);

			float weight = nextHeight / (nextHeight - prevHeight);
			texCoords = prevTexCoords * weight + (1.0f - weight) * texCoords;
		}

		/***********************************************
		MARKING SCHEME: Normal Mapping
		DESCRIPTION: Map sampling, normal value decompression
		***********************************************/
		texNormal = txNormal.Sample(samLinear, texCoords);
		texNormal = mul(texNormal, 2) - 1;
		texNormal = normalize(texNormal);
	}

	if (texCoords.x > 1.0 || texCoords.y > 1.0 || texCoords.x < 0.0 || texCoords.y < 0.0)
		discard;

	LightingResult lit = ComputeLighting(texNormal.xyz, IN.eyeVectorTS, IN.lightVectorTS);

	float4 emissive = Material.Emissive;
	float4 ambient = Material.Ambient * GlobalAmbient;
	float4 diffuse = Material.Diffuse * lit.Diffuse;
	float4 specular = Material.Specular * lit.Specular;

	float4 texColor = txDiffuse.Sample(samLinear, texCoords);

	float shadowMultiplier;
	if (Material.UseTexture == 4)
	{
		shadowMultiplier = ParallaxSelfShadowing(normalize(IN.lightVectorTS), texCoords, parallaxScale);
	}
	else
	{
		shadowMultiplier = 1.0f;
	}
	return (emissive + ambient + diffuse * shadowMultiplier + specular * shadowMultiplier) * texColor;
}

float4 PS(PS_INPUT IN) : SV_TARGET
{
	return Standard_PS(IN);
}

float4 PS_BILL(RTT_PS_INPUT IN) : SV_TARGET
{
	float4 texColor = txDiffuse.Sample(samLinear, IN.Tex);

	return texColor;
}

float4 RTT_PS(RTT_PS_INPUT IN) : SV_TARGET
{
	float4 texColor = txDiffuse.Sample(samLinear, IN.Tex);
	float4 bloomColor = txBloom.Sample(samLinear, IN.Tex);

	return texColor + bloomColor;
}

float4 PS_Blur(RTT_PS_INPUT IN) : SV_TARGET
{
	/***********************************************
	MARKING SCHEME: Advanced graphics techniques
	DESCRIPTION: Gaussian blur, Motion blur, Bloom
	***********************************************/

	// Bloom code based on learnopengl.com/Advanced-Lighting/Bloom
	const float weights[10] = { 0.407027, 0.287027f, 0.2605, 0.23945946f, 0.2081, 
								0.1616216f, 0.1526, 0.114054f, 0.0952, 0.066216f };

	int r = 1;
	int width, height;
	txDiffuse.GetDimensions(width, height);
	float2 pixelSize;
	if (isHorizontal == 1)
	{
		pixelSize = float2(1.0f / width, 0.0f);
		r = min(max(abs(mouseChange.x), 1), 10);
	}
	else
	{
		pixelSize = float2(0.0f, 1.0f / height);
		r = min(max(abs(mouseChange.y), 1), 10);
	}
	float4 bloomColor;
	float4 totalBloom = float4(0.0f, 0.0f, 0.0f, 0.0f);
	float2 texCoords;
	float d = 0;
	for (int i = 0; i < r; ++i)
	{
		texCoords = IN.Tex + float2(pixelSize.x * i, pixelSize.y * i);
		bloomColor = txDiffuse.Sample(samLinear, texCoords);
		totalBloom += bloomColor * weights[i];

		texCoords = IN.Tex - float2(pixelSize.x * i, pixelSize.y * i);
		bloomColor = txDiffuse.Sample(samLinear, texCoords);
		totalBloom += bloomColor * weights[i];

		d += weights[i];
	}

	return totalBloom / d / 2;
}

float4 PS_Depth(RTT_PS_INPUT IN) : SV_TARGET
{
	float depth = IN.Pos.z / IN.Pos.w;
	return float4(depth, depth, depth, 1.0f);
}

float4 PS_Tint(RTT_PS_INPUT IN) : SV_TARGET
{
	/***********************************************
	MARKING SCHEME: Simple screen space effect
	DESCRIPTION: Inverse colour tint
	***********************************************/
	float4 texColor = txDiffuse.Sample(samLinear, IN.Tex);

	return float4(1 - texColor.r, 1 - texColor.g, 1 - texColor.b, 1 - texColor.a);
}

//--------------------------------------------------------------------------------------
// PSSolid - render a solid color
//--------------------------------------------------------------------------------------
float4 PSSolid(PS_INPUT input) : SV_Target
{
	return vOutputColor;
}