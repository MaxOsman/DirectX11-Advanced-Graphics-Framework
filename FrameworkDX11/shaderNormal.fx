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
	bool    UseTexture;     // 4 bytes
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

//struct GS_BILL_INPUT
//{
//	float4 worldPos : POSITION;
//};

struct PS_INPUT
{
    float4 Pos : SV_POSITION;
	float4 worldPos : POSITION;
	float3 Norm : NORMAL;
	float2 Tex : TEXCOORD0;
	float3x3 Tbn : TBN;
	/*float3 eyePosTS : POSITION2;
	float3 posTS : POSITION3;*/
};

struct RTT_PS_INPUT
{
	float4 Pos : SV_POSITION;
	float2 Tex : TEXCOORD0;
};

float4 DoDiffuse(Light light, float3 L, float3 N)
{
	float NdotL = max(0, dot(N, L));
	return light.Color * NdotL;
}

float4 DoSpecular(Light lightObject, float3 vertexToEye, float3 lightDirectionToVertex, float3 Normal)
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

LightingResult DoPointLight(Light light, float3 vertexToEye, float4 vertexPos, float3 N)
{
	LightingResult result;

	float3 LightDirectionToVertex = (vertexPos - light.Position).xyz;
	float distance = length(LightDirectionToVertex);
	LightDirectionToVertex = LightDirectionToVertex / distance;

	float3 vertexToLight = (light.Position - vertexPos).xyz;
	distance = length(vertexToLight);
	vertexToLight = vertexToLight / distance;

	float attenuation = DoAttenuation(light, distance);
	attenuation = 1;

	result.Diffuse = DoDiffuse(light, vertexToLight, N) * attenuation;
	result.Specular = DoSpecular(light, vertexToEye, LightDirectionToVertex, N) * attenuation;

	return result;
}

LightingResult ComputeLighting(float4 vertexPos, float3 N)
{
	float3 vertexToEye = normalize(EyePosition - vertexPos).xyz;

	LightingResult totalResult = { { 0, 0, 0, 0 }, { 0, 0, 0, 0 } };

	[unroll]
	for (int i = 0; i < MAX_LIGHTS; ++i)
	{
		LightingResult result = { { 0, 0, 0, 0 }, { 0, 0, 0, 0 } };

		if (!Lights[i].Enabled)
			continue;

		result = DoPointLight(Lights[i], vertexToEye, vertexPos, N);

		totalResult.Diffuse += result.Diffuse;
		totalResult.Specular += result.Specular;
	}

	totalResult.Diffuse = saturate(totalResult.Diffuse);
	totalResult.Specular = saturate(totalResult.Specular);

	return totalResult;
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

//RTT_PS_INPUT BILL_VS( RTT_VS_INPUT input )
//{
//	RTT_PS_INPUT output = (RTT_PS_INPUT)0;
//
//	output.Pos = input.Pos;
//	output.Tex = float2(0.0f, 0.0f);
//
//	return output;
//}

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
		output.Tbn = TBN;

		// Week 3 lecture slides
		/*TBN = transpose(TBN);
		output.eyePosTS = normalize(mul(EyePosition.xyz, TBN));
		output.posTS = normalize(mul(output.worldPos.xyz, TBN));*/

		OutputStream.Append(output);
	}
}

// Based on www.braynzarsoft.net/viewtutorial/q16390-36-billboarding-geometry-shader
[maxvertexcount(4)]
void GS_BILL(point RTT_PS_INPUT input[1], inout TriangleStream<RTT_PS_INPUT> OutputStream)
{
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

//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------

float4 PS(PS_INPUT IN) : SV_TARGET
{
	float4 texNormal = { 0, 0, 1, 0 };
	float2 texCoords = IN.Tex;
	if (Material.UseTexture)
	{
		/*float3x3 TBNINV = transpose(IN.Tbn);
		float3 eyePosTS = normalize(mul(EyePosition.xyz, TBNINV));
		float3 posTS = normalize(mul(IN.worldPos.xyz, TBNINV));
		float3 viewDir = normalize(IN.eyePosTS - IN.posTS);
		float height = txParallax.Sample(samLinear, IN.Tex).x;
		texCoords = IN.Tex - float2( viewDir.xy / viewDir.z * (height * 0.1f) );*/

		texNormal = txNormal.Sample(samLinear, IN.Tex);
		texNormal = mul(texNormal, 2) - 1;
		texNormal = normalize(texNormal);
	}

	LightingResult lit = ComputeLighting(IN.worldPos, normalize(mul(texNormal.xyz, IN.Tbn)));

	float4 emissive = Material.Emissive;
	float4 ambient = Material.Ambient * GlobalAmbient;
	float4 diffuse = Material.Diffuse * lit.Diffuse;
	float4 specular = Material.Specular * lit.Specular;

	float4 texColor = { 1, 1, 1, 1 };
	if (Material.UseTexture)
	{
		texColor = txDiffuse.Sample(samLinear, texCoords);
	}

	return (emissive + ambient + diffuse + specular) * texColor;
}

float4 RTT_PS(RTT_PS_INPUT IN) : SV_TARGET
{
	float4 texColor = txDiffuse.Sample(samLinear, IN.Tex);

	return texColor;
}

//--------------------------------------------------------------------------------------
// PSSolid - render a solid color
//--------------------------------------------------------------------------------------
float4 PSSolid(PS_INPUT input) : SV_Target
{
	return vOutputColor;
}