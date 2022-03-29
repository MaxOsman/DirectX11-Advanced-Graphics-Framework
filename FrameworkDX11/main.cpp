//--------------------------------------------------------------------------------------
// File: main.cpp
//
// This application demonstrates animation using matrix transformations
//
// http://msdn.microsoft.com/en-us/library/windows/apps/ff729722.aspx
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#define _XM_NO_INTRINSICS_

#include "main.h"
#include "CubeGameObject.h"
#include "TerrainGameObject.h"
#include "Camera.h"
#include "Debug.h"
#include "Spline.h"

//--------------------------------------------------------------------------------------
// Entry point to the program. Initializes everything and goes into a message processing 
// loop. Idle time is used to render the scene.
//--------------------------------------------------------------------------------------
int WINAPI wWinMain( _In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow )
{
    UNREFERENCED_PARAMETER( hPrevInstance );
    UNREFERENCED_PARAMETER( lpCmdLine );

    if( FAILED( InitWindow( hInstance, nCmdShow ) ) )
        return 0;

    if( FAILED( InitDevice() ) )
    {
        CleanupDevice();
        return 0;
    }

    // Main message loop
    MSG msg = {0};
    while( WM_QUIT != msg.message )
    {
        if( PeekMessage( &msg, nullptr, 0, 0, PM_REMOVE ) )
        {
            TranslateMessage( &msg );
            DispatchMessage( &msg );
        }
        else
        {
            Render();
        }
    }

    CleanupDevice();

    return ( int )msg.wParam;
}

//--------------------------------------------------------------------------------------
// Register class and create window
//--------------------------------------------------------------------------------------
HRESULT InitWindow( HINSTANCE hInstance, int nCmdShow )
{
    // Register class
    WNDCLASSEX wcex;
    wcex.cbSize = sizeof( WNDCLASSEX );
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon( hInstance, ( LPCTSTR )IDI_TUTORIAL1 );
    wcex.hCursor = LoadCursor( nullptr, IDC_ARROW );
    wcex.hbrBackground = ( HBRUSH )( COLOR_WINDOW + 1 );
    wcex.lpszMenuName = nullptr;
    wcex.lpszClassName = L"TutorialWindowClass";
    wcex.hIconSm = LoadIcon( wcex.hInstance, ( LPCTSTR )IDI_TUTORIAL1 );
    if( !RegisterClassEx( &wcex ) )
        return E_FAIL;

    // Create window
    g_hInst = hInstance;
    RECT rc = { 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT };

	g_viewWidth = WINDOW_WIDTH;
	g_viewHeight = WINDOW_HEIGHT;

    AdjustWindowRect( &rc, WS_OVERLAPPEDWINDOW, FALSE );
    g_hWnd = CreateWindow( L"TutorialWindowClass", L"Direct3D 11 Tutorial 5",
                           WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
                           CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, nullptr, nullptr, hInstance,
                           nullptr );
    if( !g_hWnd )
        return E_FAIL;

    ShowWindow( g_hWnd, nCmdShow );

    return S_OK;
}

//--------------------------------------------------------------------------------------
// Helper for compiling shaders with D3DCompile
//
// With VS 11, we could load up prebuilt .cso files instead...
//--------------------------------------------------------------------------------------
HRESULT CompileShaderFromFile( const WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut )
{
    HRESULT hr = S_OK;

    DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
    // Set the D3DCOMPILE_DEBUG flag to embed debug information in the shaders.
    // Setting this flag improves the shader debugging experience, but still allows 
    // the shaders to be optimized and to run exactly the way they will run in 
    // the release configuration of this program.
    dwShaderFlags |= D3DCOMPILE_DEBUG;

    // Disable optimizations to further improve shader debugging
    dwShaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    ID3DBlob* pErrorBlob = nullptr;
    hr = D3DCompileFromFile( szFileName, nullptr, nullptr, szEntryPoint, szShaderModel, 
        dwShaderFlags, 0, ppBlobOut, &pErrorBlob );
    if( FAILED(hr) )
    {
        if( pErrorBlob )
        {
            OutputDebugStringA( reinterpret_cast<const char*>( pErrorBlob->GetBufferPointer() ) );
            pErrorBlob->Release();
        }
        return hr;
    }
    if( pErrorBlob ) pErrorBlob->Release();

    return S_OK;
}

//--------------------------------------------------------------------------------------
// Create Direct3D device and swap chain
//--------------------------------------------------------------------------------------
HRESULT InitDevice()
{
    HRESULT hr = S_OK;

    RECT rc;
    GetClientRect( g_hWnd, &rc );
    UINT width = rc.right - rc.left;
    UINT height = rc.bottom - rc.top;

    UINT createDeviceFlags = 0;
#ifdef _DEBUG
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_DRIVER_TYPE driverTypes[] =
    {
        D3D_DRIVER_TYPE_HARDWARE,
        D3D_DRIVER_TYPE_WARP,
        D3D_DRIVER_TYPE_REFERENCE,
    };
    UINT numDriverTypes = ARRAYSIZE( driverTypes );

	D3D_FEATURE_LEVEL featureLevels[] =
    {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
	};
	UINT numFeatureLevels = ARRAYSIZE( featureLevels );

    for( UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++ )
    {
        g_driverType = driverTypes[driverTypeIndex];
        hr = D3D11CreateDevice( nullptr, g_driverType, nullptr, createDeviceFlags, featureLevels, numFeatureLevels,
                                D3D11_SDK_VERSION, &g_pd3dDevice, &g_featureLevel, &g_pImmediateContext );

        if ( hr == E_INVALIDARG )
        {
            // DirectX 11.0 platforms will not recognize D3D_FEATURE_LEVEL_11_1 so we need to retry without it
            hr = D3D11CreateDevice( nullptr, g_driverType, nullptr, createDeviceFlags, &featureLevels[1], numFeatureLevels - 1,
                                    D3D11_SDK_VERSION, &g_pd3dDevice, &g_featureLevel, &g_pImmediateContext );
        }

        if( SUCCEEDED( hr ) )
            break;
    }
    if( FAILED( hr ) )
        return hr;

    // Obtain DXGI factory from device (since we used nullptr for pAdapter above)
    IDXGIFactory1* dxgiFactory = nullptr;
    {
        IDXGIDevice* dxgiDevice = nullptr;
        hr = g_pd3dDevice->QueryInterface( __uuidof(IDXGIDevice), reinterpret_cast<void**>(&dxgiDevice) );
        if (SUCCEEDED(hr))
        {
            IDXGIAdapter* adapter = nullptr;
            hr = dxgiDevice->GetAdapter(&adapter);
            if (SUCCEEDED(hr))
            {
                hr = adapter->GetParent( __uuidof(IDXGIFactory1), reinterpret_cast<void**>(&dxgiFactory) );
                adapter->Release();
            }
            dxgiDevice->Release();
        }
    }
    if (FAILED(hr))
        return hr;

    UINT maxQuality = 16;
    UINT sampleCount = 1;

    // Create swap chain
    IDXGIFactory2* dxgiFactory2 = nullptr;
    hr = dxgiFactory->QueryInterface( __uuidof(IDXGIFactory2), reinterpret_cast<void**>(&dxgiFactory2) );
    if ( dxgiFactory2 )
    {
        // DirectX 11.1 or later
        hr = g_pd3dDevice->QueryInterface( __uuidof(ID3D11Device1), reinterpret_cast<void**>(&g_pd3dDevice1) );
        if (SUCCEEDED(hr))
        {
            (void) g_pImmediateContext->QueryInterface( __uuidof(ID3D11DeviceContext1), reinterpret_cast<void**>(&g_pImmediateContext1) );
        }

        DXGI_SWAP_CHAIN_DESC1 sd = {};
        sd.Width = width;
        sd.Height = height;
		sd.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;//  DXGI_FORMAT_R16G16B16A16_FLOAT;////DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.BufferCount = 1;

        sd.SampleDesc.Count = sampleCount;
        hr = g_pd3dDevice->CheckMultisampleQualityLevels(sd.Format, sd.SampleDesc.Count, &maxQuality);
        maxQuality = (maxQuality > 0 ? maxQuality - 1 : maxQuality);
        sd.SampleDesc.Quality = maxQuality;

        hr = dxgiFactory2->CreateSwapChainForHwnd( g_pd3dDevice, g_hWnd, &sd, nullptr, nullptr, &g_pSwapChain1 );
        if (SUCCEEDED(hr))
        {
            hr = g_pSwapChain1->QueryInterface( __uuidof(IDXGISwapChain), reinterpret_cast<void**>(&g_pSwapChain) );
        }

        dxgiFactory2->Release();
    }
    else
    {
        // DirectX 11.0 systems
        DXGI_SWAP_CHAIN_DESC sd = {};
        sd.BufferCount = 1;
        sd.BufferDesc.Width = width;
        sd.BufferDesc.Height = height;
        sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.BufferDesc.RefreshRate.Numerator = 60;
        sd.BufferDesc.RefreshRate.Denominator = 1;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.OutputWindow = g_hWnd;
        sd.Windowed = TRUE;

        sd.SampleDesc.Count = sampleCount;
        hr = g_pd3dDevice->CheckMultisampleQualityLevels(sd.BufferDesc.Format, sd.SampleDesc.Count, &maxQuality);
        maxQuality = (maxQuality > 0 ? maxQuality - 1 : maxQuality);
        sd.SampleDesc.Quality = maxQuality;

        hr = dxgiFactory->CreateSwapChain( g_pd3dDevice, &sd, &g_pSwapChain );
    }

    // Note this tutorial doesn't handle full-screen swapchains so we block the ALT+ENTER shortcut
    dxgiFactory->MakeWindowAssociation( g_hWnd, DXGI_MWA_NO_ALT_ENTER );

    dxgiFactory->Release();

    if (FAILED(hr))
        return hr;

    // Create a render target view
    ID3D11Texture2D* pBackBuffer = nullptr;
    hr = g_pSwapChain->GetBuffer( 0, __uuidof( ID3D11Texture2D ), reinterpret_cast<void**>( &pBackBuffer ) );
    if( FAILED( hr ) )
        return hr;

    hr = g_pd3dDevice->CreateRenderTargetView( pBackBuffer, nullptr, &g_pRenderTargetView );
    pBackBuffer->Release();
    if( FAILED( hr ) )
        return hr;

    g_wfdescNormal = {};
    g_wfdescNormal.AntialiasedLineEnable = true;
    g_wfdescNormal.CullMode = D3D11_CULL_BACK;
    g_wfdescNormal.DepthBias = 0;
    g_wfdescNormal.DepthBiasClamp = 0.0f;
    g_wfdescNormal.DepthClipEnable = true;
    g_wfdescNormal.FillMode = D3D11_FILL_SOLID;
    g_wfdescNormal.FrontCounterClockwise = false;
    g_wfdescNormal.MultisampleEnable = true;
    g_wfdescNormal.ScissorEnable = false;
    g_wfdescNormal.SlopeScaledDepthBias = 0.0f;

    g_wfdescWireframe = {};
    g_wfdescWireframe.AntialiasedLineEnable = true;
    g_wfdescWireframe.CullMode = D3D11_CULL_NONE;
    g_wfdescWireframe.DepthBias = 0;
    g_wfdescWireframe.DepthBiasClamp = 0.0f;
    g_wfdescWireframe.DepthClipEnable = true;
    g_wfdescWireframe.FillMode = D3D11_FILL_WIREFRAME;
    g_wfdescWireframe.FrontCounterClockwise = false;
    g_wfdescWireframe.MultisampleEnable = true;
    g_wfdescWireframe.ScissorEnable = false;
    g_wfdescWireframe.SlopeScaledDepthBias = 0.0f;

    // Week 6 - Render to texture
    // Code based on www.braynzarsoft.net/viewtutorial/q16390-35-render-to-texture
    D3D11_TEXTURE2D_DESC textureDesc = {};
    textureDesc.Width = width;
    textureDesc.Height = height;
    textureDesc.MipLevels = 1;
    textureDesc.ArraySize = 1;
    textureDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.Usage = D3D11_USAGE_DEFAULT;
    textureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    textureDesc.SampleDesc.Count = sampleCount;
    textureDesc.SampleDesc.Quality = maxQuality;
    g_pd3dDevice->CreateTexture2D(&textureDesc, nullptr, &g_RTTTexture.texture);

    // Depth mapping - texture
    textureDesc.SampleDesc.Count = 1;
    textureDesc.SampleDesc.Quality = 0;
    g_pd3dDevice->CreateTexture2D(&textureDesc, nullptr, &g_DepthTexture.texture);
    g_pd3dDevice->CreateTexture2D(&textureDesc, nullptr, &g_BloomTexture.texture);
    g_pd3dDevice->CreateTexture2D(&textureDesc, nullptr, &g_BlurTextureHorizontal.texture);
    g_pd3dDevice->CreateTexture2D(&textureDesc, nullptr, &g_BlurTextureVertical.texture);

    // No MSAA - texture
    g_pd3dDevice->CreateTexture2D(&textureDesc, nullptr, &g_NoMSAARTTTexture.texture);

    D3D11_RENDER_TARGET_VIEW_DESC renderTargetViewDesc = {};
    renderTargetViewDesc.Format = textureDesc.Format;
    renderTargetViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMS;
    hr = g_pd3dDevice->CreateRenderTargetView(g_RTTTexture.texture, &renderTargetViewDesc, &g_RTTTexture.view);

    // Depth mapping - target
    renderTargetViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
    hr = g_pd3dDevice->CreateRenderTargetView(g_DepthTexture.texture, &renderTargetViewDesc, &g_DepthTexture.view);
    hr = g_pd3dDevice->CreateRenderTargetView(g_BloomTexture.texture, &renderTargetViewDesc, &g_BloomTexture.view);
    hr = g_pd3dDevice->CreateRenderTargetView(g_BlurTextureHorizontal.texture, &renderTargetViewDesc, &g_BlurTextureHorizontal.view);
    hr = g_pd3dDevice->CreateRenderTargetView(g_BlurTextureVertical.texture, &renderTargetViewDesc, &g_BlurTextureVertical.view);

    // No MSAA - target
    hr = g_pd3dDevice->CreateRenderTargetView(g_NoMSAARTTTexture.texture, &renderTargetViewDesc, &g_NoMSAARTTTexture.view);

    D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc = {};
    shaderResourceViewDesc.Format = textureDesc.Format;
    shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMS;
    shaderResourceViewDesc.Texture2D.MipLevels = 1;
    hr = g_pd3dDevice->CreateShaderResourceView(g_RTTTexture.texture, &shaderResourceViewDesc, &g_RTTTexture.resource);

    // Depth mapping - resource
    shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    hr = g_pd3dDevice->CreateShaderResourceView(g_DepthTexture.texture, &shaderResourceViewDesc, &g_DepthTexture.resource);
    hr = g_pd3dDevice->CreateShaderResourceView(g_BloomTexture.texture, &shaderResourceViewDesc, &g_BloomTexture.resource);
    hr = g_pd3dDevice->CreateShaderResourceView(g_BlurTextureHorizontal.texture, &shaderResourceViewDesc, &g_BlurTextureHorizontal.resource);
    hr = g_pd3dDevice->CreateShaderResourceView(g_BlurTextureVertical.texture, &shaderResourceViewDesc, &g_BlurTextureVertical.resource);

    // No MSAA - resource
    hr = g_pd3dDevice->CreateShaderResourceView(g_NoMSAARTTTexture.texture, &shaderResourceViewDesc, &g_NoMSAARTTTexture.resource);

    // Default scene
    // Create depth stencil texture
    D3D11_TEXTURE2D_DESC descDepth = {};
    descDepth.Width = width;
    descDepth.Height = height;
    descDepth.MipLevels = 1;
    descDepth.ArraySize = 1;
    descDepth.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    descDepth.SampleDesc.Count = sampleCount;
    descDepth.SampleDesc.Quality = maxQuality;
    descDepth.Usage = D3D11_USAGE_DEFAULT;
    descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    hr = g_pd3dDevice->CreateTexture2D(&descDepth, nullptr, &g_pDepthStencilTexture);

    // No MSAA - stencil
    descDepth.SampleDesc.Count = 1;
    descDepth.SampleDesc.Quality = 0;
    hr = g_pd3dDevice->CreateTexture2D(&descDepth, nullptr, &g_pNoMSAADepthStencilTexture);

    // Create the depth stencil view
    D3D11_DEPTH_STENCIL_VIEW_DESC descDSV = {};
    descDSV.Format = descDepth.Format;
    descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;
    hr = g_pd3dDevice->CreateDepthStencilView(g_pDepthStencilTexture, &descDSV, &g_pDepthStencilView);
    
    // No MSAA - stencil view
    descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    hr = g_pd3dDevice->CreateDepthStencilView(g_pNoMSAADepthStencilTexture, &descDSV, &g_pNoMSAARTTStencilView);

    // Setup the viewport
    D3D11_VIEWPORT vp;
    vp.Width = (FLOAT)width;
    vp.Height = (FLOAT)height;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    g_pImmediateContext->RSSetViewports( 1, &vp );

	hr = InitMesh();
	if (FAILED(hr))
	{
		MessageBox(nullptr,
			L"Failed to initialise mesh.", L"Error", MB_OK);
		return hr;
	}

	hr = InitWorld(width, height);
	if (FAILED(hr))
	{
		MessageBox(nullptr,
			L"Failed to initialise world.", L"Error", MB_OK);
		return hr;
	}

	hr = g_pGameObject->initMesh(g_pd3dDevice, g_pImmediateContext);
	if (FAILED(hr))
		return hr;

    hr = g_pTerrainObject->initMesh(g_pd3dDevice, g_pImmediateContext);
    if (FAILED(hr))
        return hr;

    // ImGui Setup
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    ImGui_ImplWin32_Init(g_hWnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pImmediateContext);
    ImGui::StyleColorsClassic();

    return S_OK;
}

// ***************************************************************************************
// InitMesh
// ***************************************************************************************
HRESULT	InitMesh()
{
	// Compile the vertex shader
	ID3DBlob* pVSBlob = nullptr;
	HRESULT hr = CompileShaderFromFile(L"shader.fx", "VS", "vs_5_0", &pVSBlob);
	if (FAILED(hr))
	{
		MessageBox(nullptr, L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
		return hr;
	}
	// Create the vertex shader
	hr = g_pd3dDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, &g_pVertexShader);
	if (FAILED(hr))
	{
		pVSBlob->Release();
		return hr;
	}

    // Compile the RTT vertex shader
    ID3DBlob* pRTTVSBlob = nullptr;
    hr = CompileShaderFromFile(L"shader.fx", "RTT_VS", "vs_5_0", &pRTTVSBlob);
    if (FAILED(hr))
    {
        MessageBox(nullptr, L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
        return hr;
    }
    // Create the RTT vertex shader
    hr = g_pd3dDevice->CreateVertexShader(pRTTVSBlob->GetBufferPointer(), pRTTVSBlob->GetBufferSize(), nullptr, &g_pQuadVS);
    if (FAILED(hr))
    {
        pRTTVSBlob->Release();
        return hr;
    }

    // Compile the terrain vertex shader
    ID3DBlob* pTerrainVSBlob = nullptr;
    hr = CompileShaderFromFile(L"shader.fx", "Terrain_VS", "vs_5_0", &pTerrainVSBlob);
    if (FAILED(hr))
    {
        MessageBox(nullptr, L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
        return hr;
    }
    // Create the RTT vertex shader
    hr = g_pd3dDevice->CreateVertexShader(pTerrainVSBlob->GetBufferPointer(), pTerrainVSBlob->GetBufferSize(), nullptr, &g_pTerrainVS);
    if (FAILED(hr))
    {
        pTerrainVSBlob->Release();
        return hr;
    }


    // Compile the hull shader
    ID3DBlob* pHSBlob = nullptr;
    hr = CompileShaderFromFile(L"shader.fx", "HS", "hs_5_0", &pHSBlob);
    if (FAILED(hr))
    {
        MessageBox(nullptr, L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
        return hr;
    }
    // Create the hull shader
    hr = g_pd3dDevice->CreateHullShader(pHSBlob->GetBufferPointer(), pHSBlob->GetBufferSize(), nullptr, &g_pHullShader);
    if (FAILED(hr))
    {
        pHSBlob->Release();
        return hr;
    }

    // Compile the domain shader
    ID3DBlob* pDSBlob = nullptr;
    hr = CompileShaderFromFile(L"shader.fx", "DS", "ds_5_0", &pDSBlob);
    if (FAILED(hr))
    {
        MessageBox(nullptr, L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
        return hr;
    }
    // Create the domain shader
    hr = g_pd3dDevice->CreateDomainShader(pDSBlob->GetBufferPointer(), pDSBlob->GetBufferSize(), nullptr, &g_pDomainShader);
    if (FAILED(hr))
    {
        pDSBlob->Release();
        return hr;
    }


    // Set up geometry shader
    ID3DBlob* pGSBlob = nullptr;
    hr = CompileShaderFromFile(L"shader.fx", "GS", "gs_5_0", &pGSBlob);
    if (FAILED(hr))
    {
        MessageBox(nullptr, L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
        return hr;
    }
    hr = g_pd3dDevice->CreateGeometryShader(pGSBlob->GetBufferPointer(), pGSBlob->GetBufferSize(), nullptr, &g_GeometryShader);
    pGSBlob->Release();
    if (FAILED(hr))
        return hr;

    // Set up geometry billboarding shader
    ID3DBlob* pGSBillBlob = nullptr;
    hr = CompileShaderFromFile(L"shader.fx", "GS_BILL", "gs_5_0", &pGSBillBlob);
    if (FAILED(hr))
    {
        MessageBox(nullptr, L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
        return hr;
    }
    hr = g_pd3dDevice->CreateGeometryShader(pGSBillBlob->GetBufferPointer(), pGSBillBlob->GetBufferSize(), nullptr, &g_pGeometryBillboardShader);
    if (FAILED(hr))
    {
        pGSBillBlob->Release();
        return hr;
    }

    // Compile the depth geometry shader
    ID3DBlob* pDepthGSBlob = nullptr;
    hr = CompileShaderFromFile(L"shader.fx", "GS_Depth", "gs_5_0", &pDepthGSBlob);
    if (FAILED(hr))
    {
        MessageBox(nullptr, L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
        return hr;
    }
    // Create the depth geometry shader
    hr = g_pd3dDevice->CreateGeometryShader(pDepthGSBlob->GetBufferPointer(), pDepthGSBlob->GetBufferSize(), nullptr, &g_pDepthGS);
    if (FAILED(hr))
    {
        pDepthGSBlob->Release();
        return hr;
    }


	// Define the input layout
	D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT , D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT , D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "BINORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	UINT numElements = ARRAYSIZE(layout);
	// Create the input layout
	hr = g_pd3dDevice->CreateInputLayout(layout, numElements, pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), &g_pVertexLayout);
	pVSBlob->Release();
	if (FAILED(hr))
		return hr;

    // Define the RTT input layout
    D3D11_INPUT_ELEMENT_DESC RTTlayout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT , D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    UINT RRTNumElements = ARRAYSIZE(RTTlayout);
    // Create the RTT input layout
    hr = g_pd3dDevice->CreateInputLayout(RTTlayout, RRTNumElements, pRTTVSBlob->GetBufferPointer(), pRTTVSBlob->GetBufferSize(), &g_pQuadLayout);
    pRTTVSBlob->Release();
    if (FAILED(hr))
        return hr;


	// Compile the pixel shader
	ID3DBlob* pPSBlob = nullptr;
	hr = CompileShaderFromFile(L"shader.fx", "PS", "ps_5_0", &pPSBlob);
	if (FAILED(hr))
	{
		MessageBox(nullptr, L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
		return hr;
	}
	// Create the pixel shader
	hr = g_pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &g_pPixelShader);
	pPSBlob->Release();
	if (FAILED(hr))
		return hr;

    // Compile the billboarding pixel shader
    ID3DBlob* pPSBillBlob = nullptr;
    hr = CompileShaderFromFile(L"shader.fx", "PS_BILL", "ps_5_0", &pPSBillBlob);
    if (FAILED(hr))
    {
        MessageBox(nullptr, L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
        return hr;
    }
    // Create the billboarding pixel shader
    hr = g_pd3dDevice->CreatePixelShader(pPSBillBlob->GetBufferPointer(), pPSBillBlob->GetBufferSize(), nullptr, &g_pBillPS);
    pPSBillBlob->Release();
    if (FAILED(hr))
        return hr;

    // Compile the depth pixel shader
    ID3DBlob* pPSDepthBlob = nullptr;
    hr = CompileShaderFromFile(L"shader.fx", "PS_Depth", "ps_5_0", &pPSDepthBlob);
    if (FAILED(hr))
    {
        MessageBox(nullptr, L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
        return hr;
    }
    // Create the depth pixel shader
    hr = g_pd3dDevice->CreatePixelShader(pPSDepthBlob->GetBufferPointer(), pPSDepthBlob->GetBufferSize(), nullptr, &g_pDepthPS);
    pPSDepthBlob->Release();
    if (FAILED(hr))
        return hr;

    // Compile the tint pixel shader
    ID3DBlob* pPSTintBlob = nullptr;
    hr = CompileShaderFromFile(L"shader.fx", "PS_Tint", "ps_5_0", &pPSTintBlob);
    if (FAILED(hr))
    {
        MessageBox(nullptr, L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
        return hr;
    }
    // Create the tint pixel shader
    hr = g_pd3dDevice->CreatePixelShader(pPSTintBlob->GetBufferPointer(), pPSTintBlob->GetBufferSize(), nullptr, &g_pTintPS);
    pPSTintBlob->Release();
    if (FAILED(hr))
        return hr;

    // Compile the blur pixel shader
    ID3DBlob* pPSBlurBlob = nullptr;
    hr = CompileShaderFromFile(L"shader.fx", "PS_Blur", "ps_5_0", &pPSBlurBlob);
    if (FAILED(hr))
    {
        MessageBox(nullptr, L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
        return hr;
    }
    // Create the blur pixel shader
    hr = g_pd3dDevice->CreatePixelShader(pPSBlurBlob->GetBufferPointer(), pPSBlurBlob->GetBufferSize(), nullptr, &g_pBlurPS);
    pPSBlurBlob->Release();
    if (FAILED(hr))
        return hr;

    // Compile the RTT pixel shader
    ID3DBlob* pPSRTTBlob = nullptr;
    hr = CompileShaderFromFile(L"shader.fx", "RTT_PS", "ps_5_0", &pPSRTTBlob);
    if (FAILED(hr))
    {
        MessageBox(nullptr, L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
        return hr;
    }
    // Create the RTT pixel shader
    hr = g_pd3dDevice->CreatePixelShader(pPSRTTBlob->GetBufferPointer(), pPSRTTBlob->GetBufferSize(), nullptr, &g_pQuadPS);
    pPSRTTBlob->Release();
    if (FAILED(hr))
        return hr;


    hr = CreateDDSTextureFromFile(g_pd3dDevice, L"Resources\\stone.dds", nullptr, &g_pSpriteTexture);


    // Screen quad
    g_ScreenQuad[0].pos = XMFLOAT3(-1.0f, 1.0f, 0.0f);
    g_ScreenQuad[0].tex = XMFLOAT2(0.0f, 0.0f);
    g_ScreenQuad[1].pos = XMFLOAT3(1.0f, 1.0f, 0.0f);
    g_ScreenQuad[1].tex = XMFLOAT2(1.0f, 0.0f);
    g_ScreenQuad[2].pos = XMFLOAT3(-1.0f, -1.0f, 0.0f);
    g_ScreenQuad[2].tex = XMFLOAT2(0.0f, 1.0f);
    g_ScreenQuad[3].pos = XMFLOAT3(1.0f, -1.0f, 0.0f);
    g_ScreenQuad[3].tex = XMFLOAT2(1.0f, 1.0f);

    D3D11_BUFFER_DESC bd = {};
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(SCREEN_VERTEX) * 4;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = 0;

    // Create RTT vertex buffer
    D3D11_SUBRESOURCE_DATA InitData = {};
    InitData.pSysMem = g_ScreenQuad;
    hr = g_pd3dDevice->CreateBuffer(&bd, &InitData, &g_pScreenQuadVB);
    if (FAILED(hr))
        return hr;

    // Sprites test
    for (unsigned short i = 0; i < 5; ++i)
    {
        for (unsigned short j = 0; j < 5; ++j)
        {
            for (unsigned short k = 0; k < 5; ++k)
            {
                g_SpriteArray[k + j * 5 + i * 25].pos = XMFLOAT3(-10.0f + 2 * i, 5.0f + 2 * j, -10.0f + 2 * k);
                g_SpriteArray[k + j * 5 + i * 25].tex = XMFLOAT2(0.0f, 0.0f);
            }
        }
    }

    //g_Spline = Spline(g_pd3dDevice);

    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(SCREEN_VERTEX) * g_numberOfSprites;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = 0;

    // Create sprite vertex buffer
    InitData.pSysMem = g_SpriteArray;
    hr = g_pd3dDevice->CreateBuffer(&bd, &InitData, &g_pSpriteVertexBuffer);

	// Create the constant buffer
	bd.ByteWidth = sizeof(ConstantBuffer);
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	hr = g_pd3dDevice->CreateBuffer(&bd, nullptr, &g_pConstantBuffer);

	// Create the light constant buffer
	bd.ByteWidth = sizeof(LightPropertiesConstantBuffer);
	hr = g_pd3dDevice->CreateBuffer(&bd, nullptr, &g_pLightConstantBuffer);

    // Create the billboard constant buffer
    bd.ByteWidth = sizeof(BillboardConstantBuffer);
    hr = g_pd3dDevice->CreateBuffer(&bd, nullptr, &g_pSpriteConstantBuffer);

    // Create the material constant buffer
    bd.ByteWidth = sizeof(MaterialPropertiesConstantBuffer);
    hr = g_pd3dDevice->CreateBuffer(&bd, nullptr, &g_pMaterialConstantBuffer);

    // Create the blur constant buffer
    bd.ByteWidth = sizeof(BlurProperties);
    hr = g_pd3dDevice->CreateBuffer(&bd, nullptr, &g_pBlurConstantBuffer);

    // Create the tesselation constant buffer
    bd.ByteWidth = sizeof(TessProperties);
    hr = g_pd3dDevice->CreateBuffer(&bd, nullptr, &g_pTessConstantBuffer);

    // Create the terrain constant buffer
    bd.ByteWidth = sizeof(TerrainProperties);
    hr = g_pd3dDevice->CreateBuffer(&bd, nullptr, &g_pTerrainConstantBuffer);

	return hr;
}

// ***************************************************************************************
// InitWorld
// ***************************************************************************************
HRESULT	InitWorld(int width, int height)
{
    g_pCamera = new Camera(WINDOW_HEIGHT, WINDOW_WIDTH, XMFLOAT3(0.0f, 0.0f, -3.0f), XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, 1.0f, 0.0f));
    g_pDebug = new Debug();
    g_pGameObject = new CubeGameObject();
    g_pTerrainObject = new TerrainGameObject();

    g_pGameObject->setPosition({ 0.0f, 0.0f, 0.0f });
    g_pTerrainObject->setPosition({ 0.0f, -10.0f, 0.0f });
    g_pTerrainObject->setScale({0.2f, 0.2f, 0.2f});

    g_LightPos = { 0.0f, 0.0f, -4.0f, 0.0f };

	return S_OK;
}

//--------------------------------------------------------------------------------------
// Clean up the objects we've created
//--------------------------------------------------------------------------------------
void CleanupDevice()
{
    g_pCamera = nullptr;
    delete g_pCamera;
    g_pDebug = nullptr;
    delete g_pDebug;

    g_pGameObject->cleanup();
    g_pGameObject = nullptr;
    delete g_pGameObject;

    // Remove any bound render target or depth/stencil buffer
    ID3D11RenderTargetView* nullViews[] = { nullptr };
    g_pImmediateContext->OMSetRenderTargets(_countof(nullViews), nullViews, nullptr);

    if( g_pImmediateContext ) g_pImmediateContext->ClearState();
    // Flush the immediate context to force cleanup
    if (g_pImmediateContext1) g_pImmediateContext1->Flush();
    g_pImmediateContext->Flush();

    if (g_pLightConstantBuffer) g_pLightConstantBuffer->Release();
    if (g_pVertexLayout) g_pVertexLayout->Release();
    if( g_pConstantBuffer ) g_pConstantBuffer->Release();
    if( g_pVertexShader ) g_pVertexShader->Release();
    if( g_pPixelShader ) g_pPixelShader->Release();
    if (g_GeometryShader) g_GeometryShader->Release();
    if( g_pDepthStencilTexture) g_pDepthStencilTexture->Release();
    if( g_pDepthStencilView ) g_pDepthStencilView->Release();
    if( g_pRenderTargetView ) g_pRenderTargetView->Release();
    if( g_pSwapChain1 ) g_pSwapChain1->Release();
    if( g_pSwapChain ) g_pSwapChain->Release();
    if( g_pImmediateContext1 ) g_pImmediateContext1->Release();
    if( g_pImmediateContext ) g_pImmediateContext->Release();
    if (g_RTTTexture.texture) g_RTTTexture.texture->Release();
    if (g_RTTTexture.view) g_RTTTexture.view->Release();
    if (g_RTTTexture.resource) g_RTTTexture.resource->Release();
    if (g_pRTTStencilView) g_pRTTStencilView->Release();
    if (g_pScreenQuadVB) g_pScreenQuadVB->Release();
    if (g_pQuadLayout) g_pQuadLayout->Release();
    if (g_pQuadVS) g_pQuadVS->Release();
    if (g_pQuadPS) g_pQuadPS->Release();
    if (g_pBillPS) g_pBillPS->Release();
    if (g_pSpriteVertexBuffer) g_pSpriteVertexBuffer->Release();
    if (g_pSpriteLayout) g_pSpriteLayout->Release();
    if (g_pGeometryBillboardShader) g_pGeometryBillboardShader->Release();
    if (g_pSpriteTexture) g_pSpriteTexture->Release();
    if (g_pSpriteConstantBuffer) g_pSpriteConstantBuffer->Release();
    if (g_DepthTexture.texture) g_DepthTexture.texture->Release();
    if (g_DepthTexture.view) g_DepthTexture.view->Release();
    if (g_DepthTexture.resource) g_DepthTexture.resource->Release();
    if (g_pDepthGS) g_pDepthGS->Release();
    if (g_pDepthPS) g_pDepthPS->Release();
    if (g_NoMSAARTTTexture.texture) g_NoMSAARTTTexture.texture->Release();
    if (g_NoMSAARTTTexture.view) g_NoMSAARTTTexture.view->Release();
    if (g_NoMSAARTTTexture.resource) g_NoMSAARTTTexture.resource->Release();
    if (g_pNoMSAARTTStencilView) g_pNoMSAARTTStencilView->Release();
    if (g_pNoMSAADepthStencilTexture) g_pNoMSAADepthStencilTexture->Release();
    if (g_pTintPS) g_pTintPS->Release();
    if (g_pMaterialConstantBuffer) g_pMaterialConstantBuffer->Release();
    if (g_pHullShader) g_pHullShader->Release();
    if (g_pDomainShader) g_pDomainShader->Release();
    if (g_pTessConstantBuffer) g_pTessConstantBuffer->Release();

    if (g_BloomTexture.texture) g_BloomTexture.texture->Release();
    if (g_BloomTexture.view) g_BloomTexture.view->Release();
    if (g_BloomTexture.resource) g_BloomTexture.resource->Release();
    if (g_BlurTextureHorizontal.texture) g_BlurTextureHorizontal.texture->Release();
    if (g_BlurTextureHorizontal.view) g_BlurTextureHorizontal.view->Release();
    if (g_BlurTextureHorizontal.resource) g_BlurTextureHorizontal.resource->Release();
    if (g_BlurTextureVertical.texture) g_BlurTextureVertical.texture->Release();
    if (g_BlurTextureVertical.view) g_BlurTextureVertical.view->Release();
    if (g_BlurTextureVertical.resource) g_BlurTextureVertical.resource->Release();

    if (g_pBlurPS) g_pBlurPS->Release();
    if (g_pBlurConstantBuffer) g_pBlurConstantBuffer->Release();
    if (g_pTerrainConstantBuffer) g_pTerrainConstantBuffer->Release();
    if (g_pTerrainVS) g_pTerrainVS->Release();

    ID3D11Debug* debugDevice = nullptr;
    g_pd3dDevice->QueryInterface(__uuidof(ID3D11Debug), reinterpret_cast<void**>(&debugDevice));

    if (g_pd3dDevice1) g_pd3dDevice1->Release();
    if (g_pd3dDevice) g_pd3dDevice->Release();

    // handy for finding dx memory leaks
    if(debugDevice)
        debugDevice->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL);

    if (debugDevice)
        debugDevice->Release();
}

//--------------------------------------------------------------------------------------
// Called every time the application receives a message
//--------------------------------------------------------------------------------------
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK WndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
    PAINTSTRUCT ps;
    HDC hdc;

    /*if (message >= WM_KEYFIRST && message <= WM_KEYLAST)
    {
        HandleKeyboard(wParam, lParam);
    }*/
    if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam))
    {
        return true;
    }
    switch( message )
    {
	case WM_LBUTTONDOWN:
	{
		int xPos = GET_X_LPARAM(lParam);
		int yPos = GET_Y_LPARAM(lParam);
		break;
	}
    case WM_RBUTTONDOWN:
    {
        g_pCamera->ChangeActive();
        if (g_pCamera->GetActive())
        {
            while (::ShowCursor(FALSE) >= 0);
            RECT rc;
            GetClientRect(g_hWnd, &rc);
            MapWindowPoints(g_hWnd, nullptr, reinterpret_cast<POINT*>(&rc), 2);
            ClipCursor(&rc);

            GetClientRect(hWnd, &rc);
            MapWindowPoints(hWnd, nullptr, reinterpret_cast<POINT*>(&rc), 2);
            SetCursorPos(0.5 * rc.right + 0.5 * rc.left, 0.5 * rc.bottom + 0.5 * rc.top);
        }
        else
        {
            while (::ShowCursor(TRUE) < 0);
            ClipCursor(nullptr);
        }
        break;
    }
    case WM_PAINT:
        hdc = BeginPaint( hWnd, &ps );
        EndPaint( hWnd, &ps );
        break;

    case WM_DESTROY:
        PostQuitMessage( 0 );
        break;

        // Note that this tutorial does not handle resizing (WM_SIZE) requests,
        // so we created the window without the resize border.

    default:
        return DefWindowProc( hWnd, message, wParam, lParam );
    }

    return 0;
}

float calculateDeltaTime()
{
    // Update our time
    static float deltaTime = 0.0f;
    static ULONGLONG timeStart = 0;
    ULONGLONG timeCur = GetTickCount64();
    if (timeStart == 0)
        timeStart = timeCur;
    deltaTime = (timeCur - timeStart) / 1000.0f;
    timeStart = timeCur;

    float FPS60 = 1.0f / 60.0f;
    static float cummulativeTime = 0;

    // cap the framerate at 60 fps 
    cummulativeTime += deltaTime;
    if (cummulativeTime >= FPS60) 
    {
        cummulativeTime = cummulativeTime - FPS60;
    }
    else 
    {
        return 0;
    }

    return deltaTime;
}

void HandlePerFrameInput(float deltaTime)
{
    if (g_pCamera->GetActive())
    {
        const float speed = 5.0f;
        if (GetAsyncKeyState('W'))
        {
            g_pCamera->CameraTranslate({ 0.0f, 0.0f, speed * deltaTime }, g_pCamera->GetPitch(), g_pCamera->GetYaw());
        }
        if (GetAsyncKeyState('A'))
        {
            g_pCamera->CameraTranslate({ -speed * deltaTime, 0.0f, 0.0f }, g_pCamera->GetPitch(), g_pCamera->GetYaw());
        }
        if (GetAsyncKeyState('S'))
        {
            g_pCamera->CameraTranslate({ 0.0f, 0.0f, -speed * deltaTime }, g_pCamera->GetPitch(), g_pCamera->GetYaw());
        }
        if (GetAsyncKeyState('D'))
        {
            g_pCamera->CameraTranslate({ speed * deltaTime, 0.0f, 0.0f }, g_pCamera->GetPitch(), g_pCamera->GetYaw());
        }
        if (GetAsyncKeyState('Q'))
        {
            g_pCamera->CameraTranslate({ 0.0f, speed * deltaTime, 0.0f }, 0, 0);
        }
        if (GetAsyncKeyState('E'))
        {
            g_pCamera->CameraTranslate({ 0.0f, -speed * deltaTime, 0.0f }, 0, 0);
        }
    }
}

void setupConstantBuffers()
{
    LightPropertiesConstantBuffer lightProperties;

    lightProperties.EyePosition = g_pCamera->GetEye();
    lightProperties.Lights[0].Enabled = static_cast<int>(true);
    lightProperties.Lights[0].LightType = PointLight;
    lightProperties.Lights[0].Color = XMFLOAT4(Colors::White);
    lightProperties.Lights[0].SpotAngle = XMConvertToRadians(45.0f);
    lightProperties.Lights[0].ConstantAttenuation = 0.18;
    lightProperties.Lights[0].LinearAttenuation = 0.18;
    lightProperties.Lights[0].QuadraticAttenuation = 0.18;
    lightProperties.Lights[0].Position = { g_LightPos.x + guiLightX, g_LightPos.y + guiLightY, g_LightPos.z + guiLightZ, 0.0f };
    XMVECTOR LightDirection = XMVectorSet(-(g_LightPos.x + guiLightX), -(g_LightPos.y + guiLightY), -(g_LightPos.z + guiLightZ), 0.0f);
    LightDirection = XMVector3Normalize(LightDirection);
    XMStoreFloat4(&lightProperties.Lights[0].Direction, LightDirection);

    g_pImmediateContext->UpdateSubresource(g_pLightConstantBuffer, 0, nullptr, &lightProperties, 0, 0);
    g_pImmediateContext->PSSetConstantBuffers(2, 1, &g_pLightConstantBuffer);
    g_pImmediateContext->DSSetConstantBuffers(2, 1, &g_pLightConstantBuffer);

    // Set up billboard
    BillboardConstantBuffer billboardProperties;
    billboardProperties.EyePos = g_pCamera->GetEye();
    billboardProperties.UpVector = g_pCamera->GetUp();
    g_pImmediateContext->UpdateSubresource(g_pSpriteConstantBuffer, 0, nullptr, &billboardProperties, 0, 0);
    g_pImmediateContext->HSSetConstantBuffers(3, 1, &g_pSpriteConstantBuffer);

    // Materials
    MaterialPropertiesConstantBuffer materialProperties;
    materialProperties.Material.Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    materialProperties.Material.Specular = XMFLOAT4(1.0f, 0.2f, 0.2f, 1.0f);
    materialProperties.Material.SpecularPower = 32.0f;
    materialProperties.Material.UseTexture = materialSelection;
    g_pImmediateContext->UpdateSubresource(g_pMaterialConstantBuffer, 0, nullptr, &materialProperties, 0, 0);
    g_pImmediateContext->PSSetConstantBuffers(1, 1, &g_pMaterialConstantBuffer);

    // Blur
    BlurProperties blurProps;
    blurProps.isHorizontal = 0;
    blurProps.mouseChange = g_pCamera->GetChange();
    blurProps.Padding = 0;
    g_pImmediateContext->UpdateSubresource(g_pBlurConstantBuffer, 0, nullptr, &blurProps, 0, 0);
    g_pImmediateContext->PSSetConstantBuffers(4, 1, &g_pBlurConstantBuffer);

    // Tesselation
    TessProperties tessProps;
    tessProps.tessFactor = g_tessFactor;
    tessProps.padding = { 0,0,0 };
    g_pImmediateContext->UpdateSubresource(g_pTessConstantBuffer, 0, nullptr, &tessProps, 0, 0);
    g_pImmediateContext->HSSetConstantBuffers(5, 1, &g_pTessConstantBuffer);

    // Terrain
    g_Terrain.IsTerrain = 0;
    g_pImmediateContext->UpdateSubresource(g_pTerrainConstantBuffer, 0, nullptr, &g_Terrain, 0, 0);
    g_pImmediateContext->PSSetConstantBuffers(6, 1, &g_pTerrainConstantBuffer);
    g_pImmediateContext->DSSetConstantBuffers(6, 1, &g_pTerrainConstantBuffer);
}

void DrawScene(ConstantBuffer* cb)
{
    g_Terrain.IsTerrain = 0;
    g_pImmediateContext->UpdateSubresource(g_pTerrainConstantBuffer, 0, nullptr, &g_Terrain, 0, 0);

    g_pImmediateContext->VSSetShader(g_pVertexShader, nullptr, 0);
    g_pImmediateContext->GSSetShader(NULL, nullptr, 0);
    g_pImmediateContext->PSSetShader(g_pPixelShader, nullptr, 0);
    g_pImmediateContext->HSSetShader(g_pHullShader, nullptr, 0);
    g_pImmediateContext->DSSetShader(g_pDomainShader, nullptr, 0);
    g_pImmediateContext->IASetInputLayout(g_pVertexLayout);

    /*XMMATRIX mGO = XMLoadFloat4x4(g_pGameObject->getTransform());
    cb->mWorld = XMMatrixTranspose(mGO);
    g_pImmediateContext->UpdateSubresource(g_pConstantBuffer, 0, nullptr, cb, 0, 0);
    g_pGameObject->draw(g_pImmediateContext);*/

    g_Terrain.IsTerrain = 1;
    g_pImmediateContext->UpdateSubresource(g_pTerrainConstantBuffer, 0, nullptr, &g_Terrain, 0, 0);

    XMMATRIX mGO = XMLoadFloat4x4(g_pTerrainObject->getTransform());
    cb->mWorld = XMMatrixTranspose(mGO);
    g_pImmediateContext->UpdateSubresource(g_pConstantBuffer, 0, nullptr, cb, 0, 0);
    g_pTerrainObject->draw(g_pImmediateContext);
}

void DrawSceneSprites()
{
    g_pImmediateContext->VSSetShader(g_pQuadVS, nullptr, 0);
    g_pImmediateContext->HSSetShader(NULL, nullptr, 0);
    g_pImmediateContext->DSSetShader(NULL, nullptr, 0);
    g_pImmediateContext->GSSetShader(g_pGeometryBillboardShader, nullptr, 0);
    g_pImmediateContext->PSSetShader(g_pBillPS, nullptr, 0);

    /*UINT stride = sizeof(SCREEN_VERTEX);
    UINT offset = 0;
    ID3D11Buffer* pSpriteBuffers[1] = { g_pSpriteVertexBuffer };
    g_pImmediateContext->IASetVertexBuffers(0, 1, pSpriteBuffers, &stride, &offset);
    g_pImmediateContext->IASetInputLayout(g_pQuadLayout);
    g_pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);*/

    /*g_pImmediateContext->PSSetShaderResources(0, 1, &g_pSpriteTexture);
    g_pImmediateContext->GSSetConstantBuffers(3, 1, &g_pSpriteConstantBuffer);
    g_pImmediateContext->Draw(g_numberOfSprites, 0);*/
}

void Bloom(ConstantBuffer* cb)
{
    /***********************************************
    MARKING SCHEME: Advanced graphics techniques
    DESCRIPTION: Gaussian blur, Motion blur
    ***********************************************/

    g_pImmediateContext->OMSetRenderTargets(1, &g_BloomTexture.view, g_pNoMSAARTTStencilView);
    g_pImmediateContext->ClearRenderTargetView(g_BloomTexture.view, Colors::MidnightBlue);
    g_pImmediateContext->ClearDepthStencilView(g_pNoMSAARTTStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

    // Draw scene to target
    DrawScene(cb);

    DrawSceneSprites();

    ////////////////////////////////////////////////////////////////////////////////////////////////////

    g_pImmediateContext->OMSetRenderTargets(1, &g_BlurTextureHorizontal.view, g_pNoMSAARTTStencilView);
    g_pImmediateContext->ClearRenderTargetView(g_BlurTextureHorizontal.view, Colors::Black);
    g_pImmediateContext->ClearDepthStencilView(g_pNoMSAARTTStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

    g_pImmediateContext->VSSetShader(g_pQuadVS, nullptr, 0);
    g_pImmediateContext->GSSetShader(NULL, nullptr, 0);
    g_pImmediateContext->PSSetShader(g_pBlurPS, nullptr, 0);

    // Render to quad for 1st (horizontal) pass
    UINT stride = sizeof(SCREEN_VERTEX);
    UINT offset = 0;
    ID3D11Buffer* pBuffers[1] = { g_pScreenQuadVB };
    g_pImmediateContext->IASetVertexBuffers(0, 1, pBuffers, &stride, &offset);

    g_pImmediateContext->IASetInputLayout(g_pQuadLayout);
    g_pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    g_pImmediateContext->PSSetShaderResources(0, 1, &g_BloomTexture.resource);

    g_pImmediateContext->Draw(4, 0);

    ////////////////////////////////////////////////////////////////////////////////////////////////////

    g_pImmediateContext->OMSetRenderTargets(1, &g_BlurTextureVertical.view, g_pNoMSAARTTStencilView);
    g_pImmediateContext->ClearRenderTargetView(g_BlurTextureVertical.view, Colors::Black);
    g_pImmediateContext->ClearDepthStencilView(g_pNoMSAARTTStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

    BlurProperties blurProps;
    blurProps.isHorizontal = 0;
    blurProps.mouseChange = g_pCamera->GetChange();
    blurProps.Padding = 0;
    g_pImmediateContext->UpdateSubresource(g_pBlurConstantBuffer, 0, nullptr, &blurProps, 0, 0);

    // Render to quad for 2nd (vertical) pass
    stride = sizeof(SCREEN_VERTEX);
    offset = 0;
    pBuffers[0] = { g_pScreenQuadVB };
    g_pImmediateContext->IASetVertexBuffers(0, 1, pBuffers, &stride, &offset);

    g_pImmediateContext->PSSetShaderResources(0, 1, &g_BlurTextureHorizontal.resource);

    g_pImmediateContext->Draw(4, 0);

    ID3D11ShaderResourceView* nullSRV = { nullptr };
    g_pImmediateContext->PSSetShaderResources(0, 1, &nullSRV);
}

void RenderScreenQuad(ConstantBuffer* cb)
{
    /***********************************************
    MARKING SCHEME: Special effects pipeline
    DESCRIPTION: Render to texture implemented
    ***********************************************/
    g_pImmediateContext->OMSetRenderTargets(1, &g_RTTTexture.view, g_pDepthStencilView);
    g_pImmediateContext->ClearRenderTargetView(g_RTTTexture.view, Colors::MidnightBlue);
    g_pImmediateContext->ClearDepthStencilView(g_pDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

    // Draw scene to RTT target
    DrawScene(cb);

    DrawSceneSprites();

    // Reset target
    g_pImmediateContext->OMSetRenderTargets(1, &g_pRenderTargetView, g_pDepthStencilView);

    // New shaders
    g_pImmediateContext->VSSetShader(g_pQuadVS, nullptr, 0);
    g_pImmediateContext->GSSetShader(NULL, nullptr, 0);
    if (guiSelection == 2)
    {
        // Inverse tint
        g_pImmediateContext->PSSetShader(g_pTintPS, nullptr, 0);
    }
    else
    {
        g_pImmediateContext->PSSetShader(g_pQuadPS, nullptr, 0);
    }

    /***********************************************
    MARKING SCHEME: Special effects pipeline
    DESCRIPTION: Render of texture to full screen quad
    ***********************************************/
    UINT stride = sizeof(SCREEN_VERTEX);
    UINT offset = 0;
    ID3D11Buffer* pBuffers[1] = { g_pScreenQuadVB };
    g_pImmediateContext->IASetVertexBuffers(0, 1, pBuffers, &stride, &offset);

    g_pImmediateContext->IASetInputLayout(g_pQuadLayout);
    g_pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    if (guiSelection == 1)
    {
        // Depth rendering
        ID3D11ShaderResourceView* nullSRV = { nullptr };
        g_pImmediateContext->PSSetShaderResources(3, 1, &nullSRV);
        g_pImmediateContext->PSSetShaderResources(0, 1, &g_DepthTexture.resource);
    }
    else
    {
        /***********************************************
        MARKING SCHEME: Advanced graphics techniques
        DESCRIPTION: MSAA
        ***********************************************/
        g_pImmediateContext->ResolveSubresource(g_NoMSAARTTTexture.texture, 0, g_RTTTexture.texture, 
                                                0, DXGI_FORMAT_R32G32B32A32_FLOAT);
        g_pImmediateContext->PSSetShaderResources(0, 1, &g_NoMSAARTTTexture.resource);
        if (guiMotionBlur)
        {
            g_pImmediateContext->PSSetShaderResources(0, 1, &g_BlurTextureVertical.resource);
        }
    }
    
    g_pImmediateContext->Draw(4, 0);
    ID3D11ShaderResourceView* nullSRV = { nullptr };
    g_pImmediateContext->PSSetShaderResources(0, 1, &nullSRV);
    g_pImmediateContext->PSSetShaderResources(3, 1, &nullSRV);
}

void DepthMap(ConstantBuffer* cb)
{
    g_pImmediateContext->ClearRenderTargetView(g_DepthTexture.view, Colors::Black);
    g_pImmediateContext->ClearDepthStencilView(g_pNoMSAARTTStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
    g_pImmediateContext->OMSetRenderTargets(1, &g_DepthTexture.view, g_pNoMSAARTTStencilView);

    // Disabled for now to prevent error messages
    // I doubt I'll need this anyway
    //DrawScene(cb);

    DrawSceneSprites();
}

//--------------------------------------------------------------------------------------
// Render a frame
//--------------------------------------------------------------------------------------
void Render()
{
    float t = calculateDeltaTime(); // capped at 60 fps
    if (t == 0.0f)
        return;

    // Draw mode
    ID3D11RasterizerState* rsstate;
    if (g_isWireframe)
    {
        g_pd3dDevice->CreateRasterizerState(&g_wfdescWireframe, &rsstate);
    }
    else
    {
        g_pd3dDevice->CreateRasterizerState(&g_wfdescNormal, &rsstate);
    }
    g_pImmediateContext->RSSetState(rsstate);

    // Update the cube transform, material etc.
    float tempT = (guiRotation ? t : 0);
    g_pGameObject->update(tempT, g_pImmediateContext);
    g_pTerrainObject->update(g_pImmediateContext);
    g_pCamera->Update(g_hWnd);
    HandlePerFrameInput(t);

    // Get the game object world transform
    XMFLOAT4X4 v = g_pCamera->GetView();
    XMFLOAT4X4 p = g_pCamera->GetProjection();

    // Store this and the view / projection in a constant buffer for the vertex shader to use
    ConstantBuffer cb1;
    cb1.vOutputColor = XMFLOAT4(0, 0, 0, 0);
    cb1.mView = XMMatrixTranspose(XMLoadFloat4x4(&v));
    cb1.mProjection = XMMatrixTranspose(XMLoadFloat4x4(&p));
    g_pImmediateContext->UpdateSubresource(g_pConstantBuffer, 0, nullptr, &cb1, 0, 0);
    g_pImmediateContext->GSSetConstantBuffers(0, 1, &g_pConstantBuffer);
    g_pImmediateContext->HSSetConstantBuffers(0, 1, &g_pConstantBuffer);
    g_pImmediateContext->DSSetConstantBuffers(0, 1, &g_pConstantBuffer);

    setupConstantBuffers();

    // Draw functions
    /*if (guiMotionBlur)
    {
        Bloom(&cb1);
    }
    DepthMap(&cb1);*/

    // Clear the back buffer
    g_pImmediateContext->ClearRenderTargetView(g_pRenderTargetView, Colors::MidnightBlue);
    g_pImmediateContext->ClearDepthStencilView(g_pDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
    g_pImmediateContext->OMSetRenderTargets(1, &g_pRenderTargetView, g_pDepthStencilView);

    DrawScene(&cb1);
    DrawSceneSprites();

    RenderScreenQuad(&cb1);

    // Spline
    //g_Spline.Render(g_pImmediateContext, g_pQuadLayout);

    // ImGui
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    float prevHeight = g_heightFactor;

    // The window
    ImGui::Begin("Options");
    static const char* items[]{ "Diffuse", "Normals", "Parallax", "Parallax Occlusion", "Self-Shadowing POM"};
    ImGui::ListBox("Shading", &materialSelection, items, ARRAYSIZE(items));
    //static const char* items2[]{ "Default", "Depth Render", "Invert Colours" };
    //ImGui::ListBox("Render Mode", &guiSelection, items2, ARRAYSIZE(items2));
    ImGui::SliderFloat("Light X Pos", &guiLightX, -5.0f, 5.0f);
    ImGui::SliderFloat("Light Y Pos", &guiLightY, -5.0f, 5.0f);
    ImGui::SliderFloat("Light Z Pos", &guiLightZ, -5.0f, 5.0f);
    //ImGui::Checkbox("Enable Motion Blur", &guiMotionBlur);
    ImGui::Checkbox("Enable Rotation", &guiRotation);
    ImGui::Checkbox("Enable Wireframe", &g_isWireframe);
    ImGui::SliderFloat("Tesselation Factor", &g_tessFactor, 0.001f, 10.0f);
    ImGui::SliderFloat("Height Factor", &g_heightFactor, 0.0f, 20.0f);
    ImGui::End();

    /*if (prevHeight != g_heightFactor)
    {
        g_pTerrainObject->SetHeight(g_heightFactor);
        g_pTerrainObject->initMesh(g_pd3dDevice, g_pImmediateContext);
    }*/

    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    // Present our back buffer to our front buffer
    g_pSwapChain->Present(0, 0);
}