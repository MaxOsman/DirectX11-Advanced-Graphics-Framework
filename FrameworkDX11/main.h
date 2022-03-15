#pragma once

#include <windows.h>
#include <windowsx.h>
#include <d3d11_1.h>
#include <d3dcompiler.h>
#include <directxmath.h>
#include <directxcolors.h>
#include <DirectXCollision.h>
#include "DDSTextureLoader.h"
#include "resource.h"
#include "structures.h"
#include <iostream>

#include "imgui-master/imgui.h"
#include "imgui-master/imgui_impl_win32.h"
#include "imgui-master/imgui_impl_dx11.h"

#include <vector>

class Camera;
class DrawableGameObject;
class CubeGameObject;
class TerrainGameObject;
class Debug;

typedef vector<DrawableGameObject*> vecDrawables;

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720

//--------------------------------------------------------------------------------------
// Global Variables
//--------------------------------------------------------------------------------------
HINSTANCE					g_hInst = nullptr;
HWND						g_hWnd = nullptr;
D3D_DRIVER_TYPE				g_driverType = D3D_DRIVER_TYPE_NULL;
D3D_FEATURE_LEVEL			g_featureLevel = D3D_FEATURE_LEVEL_11_0;
ID3D11Device*				g_pd3dDevice = nullptr;
ID3D11Device1*				g_pd3dDevice1 = nullptr;
ID3D11DeviceContext*		g_pImmediateContext = nullptr;
ID3D11DeviceContext1*		g_pImmediateContext1 = nullptr;
IDXGISwapChain*				g_pSwapChain = nullptr;
IDXGISwapChain1*			g_pSwapChain1 = nullptr;

ID3D11Texture2D*			g_pDepthStencilTexture = nullptr;
ID3D11RenderTargetView*		g_pRenderTargetView = nullptr;
ID3D11DepthStencilView*		g_pDepthStencilView = nullptr;

ID3D11VertexShader*			g_pVertexShader = nullptr;
ID3D11PixelShader*			g_pPixelShader = nullptr;
ID3D11GeometryShader*		g_GeometryShader = nullptr;

ID3D11InputLayout*			g_pVertexLayout = nullptr;
ID3D11Buffer*				g_pConstantBuffer = nullptr;
ID3D11Buffer*				g_pLightConstantBuffer = nullptr;
ID3D11Buffer*				g_pMaterialConstantBuffer = nullptr;
ID3D11Buffer*				g_pTessConstantBuffer = nullptr;

// RTT
TextureSet					g_RTTTexture;
ID3D11DepthStencilView*		g_pRTTStencilView = nullptr;

// No MSAA
TextureSet					g_NoMSAARTTTexture;
ID3D11DepthStencilView*		g_pNoMSAARTTStencilView = nullptr;
ID3D11Texture2D*			g_pNoMSAADepthStencilTexture = nullptr;

// Sprites
ID3D11GeometryShader*		g_pGeometryBillboardShader = nullptr;
ID3D11Buffer*				g_pSpriteVertexBuffer = nullptr;
ID3D11InputLayout*			g_pSpriteLayout = nullptr;
ID3D11ShaderResourceView*	g_pSpriteTexture = nullptr;
const int					g_numberOfSprites = 125;
SCREEN_VERTEX				g_SpriteArray[g_numberOfSprites];
ID3D11Buffer*				g_pSpriteConstantBuffer = nullptr;
ID3D11PixelShader*			g_pBillPS = nullptr;

ID3D11Buffer*				g_pScreenQuadVB = nullptr;
ID3D11InputLayout*			g_pQuadLayout = nullptr;
ID3D11VertexShader*			g_pQuadVS = nullptr;
ID3D11PixelShader*			g_pQuadPS = nullptr;
SCREEN_VERTEX				g_ScreenQuad[4];

// Depth Mapping
TextureSet					g_DepthTexture;
ID3D11GeometryShader*		g_pDepthGS = nullptr;
ID3D11PixelShader*			g_pDepthPS = nullptr;

ID3D11PixelShader*			g_pTintPS = nullptr;

// Bloom
TextureSet					g_BloomTexture;
TextureSet					g_BlurTextureHorizontal;
TextureSet					g_BlurTextureVertical;
ID3D11PixelShader*			g_pBlurPS = nullptr;
ID3D11Buffer*				g_pBlurConstantBuffer = nullptr;

int							g_viewWidth;
int							g_viewHeight;

CubeGameObject*				g_pGameObject;
TerrainGameObject*			g_pTerrainObject;
Camera*						g_pCamera;
Debug*						g_pDebug;
//Spline					g_Spline;
XMFLOAT4					g_LightPos;

// ImGui
int							guiSelection = 0;
int							materialSelection = 0;
bool						guiRotation = true;
bool						guiMotionBlur = false;
float						guiLightX = 0.0f;
float						guiLightY = 0.0f;
float						guiLightZ = 0.0f;

MaterialPropertiesConstantBuffer	g_Material;

ID3D11HullShader*			g_pHullShader = nullptr;
ID3D11DomainShader*			g_pDomainShader = nullptr;

D3D11_RASTERIZER_DESC		g_wfdescNormal;
D3D11_RASTERIZER_DESC		g_wfdescWireframe;
bool						g_isWireframe = false;
float						g_tessFactor = 0.1f;

ID3D11VertexShader*			g_pTerrainVS = nullptr;
TerrainProperties			g_Terrain;
ID3D11Buffer*				g_pTerrainConstantBuffer = nullptr;
float						g_heightFactor = 5.0f;

//--------------------------------------------------------------------------------------
// Forward declarations
//--------------------------------------------------------------------------------------
HRESULT		InitWindow(HINSTANCE hInstance, int nCmdShow);
HRESULT		InitDevice();
HRESULT		InitMesh();
HRESULT		InitWorld(int width, int height);
void		CleanupDevice();
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
void		Render();