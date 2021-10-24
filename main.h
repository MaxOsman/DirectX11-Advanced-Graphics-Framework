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
#include <iostream>

#include "DrawableGameObject.h"
#include "structures.h"
#include "Camera.h"
#include "Debug.h"

#include "imgui-master/imgui.h"
#include "imgui-master/imgui_impl_win32.h"
#include "imgui-master/imgui_impl_dx11.h"

#include <vector>

using namespace std;

typedef vector<DrawableGameObject*> vecDrawables;

#define WINDOW_WIDTH 960
#define WINDOW_HEIGHT 540

//--------------------------------------------------------------------------------------
// Global Variables
//--------------------------------------------------------------------------------------
HINSTANCE               g_hInst = nullptr;
HWND                    g_hWnd = nullptr;
D3D_DRIVER_TYPE         g_driverType = D3D_DRIVER_TYPE_NULL;
D3D_FEATURE_LEVEL       g_featureLevel = D3D_FEATURE_LEVEL_11_0;
ID3D11Device*			g_pd3dDevice = nullptr;
ID3D11Device1*			g_pd3dDevice1 = nullptr;
ID3D11DeviceContext*	g_pImmediateContext = nullptr;
ID3D11DeviceContext1*	g_pImmediateContext1 = nullptr;
IDXGISwapChain*			g_pSwapChain = nullptr;
IDXGISwapChain1*		g_pSwapChain1 = nullptr;
ID3D11RenderTargetView* g_pRenderTargetView = nullptr;
ID3D11Texture2D*		g_pDepthStencil = nullptr;
ID3D11DepthStencilView* g_pDepthStencilView = nullptr;
ID3D11VertexShader*		g_pVertexShader = nullptr;

ID3D11PixelShader*		g_pPixelShader = nullptr;
ID3D11InputLayout*		g_pVertexLayout = nullptr;
ID3D11Buffer*			g_pConstantBuffer = nullptr;
ID3D11Buffer*			g_pLightConstantBuffer = nullptr;

int						g_viewWidth;
int						g_viewHeight;

DrawableGameObject		g_GameObject;
Camera					g_Camera;
Debug					g_Debug;

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