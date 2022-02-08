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
    UINT sampleCount = 4;

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

    D3D11_RASTERIZER_DESC wfdesc = {};
    wfdesc.AntialiasedLineEnable = true;
    wfdesc.CullMode = D3D11_CULL_BACK;
    wfdesc.DepthBias = 0;
    wfdesc.DepthBiasClamp = 0.0f;
    wfdesc.DepthClipEnable = true;
    wfdesc.FillMode = D3D11_FILL_SOLID;
    wfdesc.FrontCounterClockwise = false;
    wfdesc.MultisampleEnable = true;
    wfdesc.ScissorEnable = false;
    wfdesc.SlopeScaledDepthBias = 0.0f;
    ID3D11RasterizerState* rsstate;
    hr = g_pd3dDevice->CreateRasterizerState(&wfdesc, &rsstate);
    g_pImmediateContext->RSSetState(rsstate);

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
    g_pd3dDevice->CreateTexture2D(&textureDesc, nullptr, &g_pRTTTexture.texture);

    // Depth mapping - texture
    textureDesc.SampleDesc.Count = 1;
    textureDesc.SampleDesc.Quality = 0;
    g_pd3dDevice->CreateTexture2D(&textureDesc, nullptr, &g_pDepthTexture.texture);
    g_pd3dDevice->CreateTexture2D(&textureDesc, nullptr, &g_pBloomTexture.texture);
    g_pd3dDevice->CreateTexture2D(&textureDesc, nullptr, &g_pBlurTextureHorizontal.texture);
    g_pd3dDevice->CreateTexture2D(&textureDesc, nullptr, &g_pBlurTextureVertical.texture);

    // No MSAA - texture
    g_pd3dDevice->CreateTexture2D(&textureDesc, nullptr, &g_pNoMSAARTTTexture.texture);

    D3D11_RENDER_TARGET_VIEW_DESC renderTargetViewDesc = {};
    renderTargetViewDesc.Format = textureDesc.Format;
    renderTargetViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMS;
    hr = g_pd3dDevice->CreateRenderTargetView(g_pRTTTexture.texture, &renderTargetViewDesc, &g_pRTTTexture.view);

    // Depth mapping - target
    renderTargetViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
    hr = g_pd3dDevice->CreateRenderTargetView(g_pDepthTexture.texture, &renderTargetViewDesc, &g_pDepthTexture.view);
    hr = g_pd3dDevice->CreateRenderTargetView(g_pBloomTexture.texture, &renderTargetViewDesc, &g_pBloomTexture.view);
    hr = g_pd3dDevice->CreateRenderTargetView(g_pBlurTextureHorizontal.texture, &renderTargetViewDesc, &g_pBlurTextureHorizontal.view);
    hr = g_pd3dDevice->CreateRenderTargetView(g_pBlurTextureVertical.texture, &renderTargetViewDesc, &g_pBlurTextureVertical.view);

    // No MSAA - target
    hr = g_pd3dDevice->CreateRenderTargetView(g_pNoMSAARTTTexture.texture, &renderTargetViewDesc, &g_pNoMSAARTTTexture.view);

    D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc = {};
    shaderResourceViewDesc.Format = textureDesc.Format;
    shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMS;
    shaderResourceViewDesc.Texture2D.MipLevels = 1;
    hr = g_pd3dDevice->CreateShaderResourceView(g_pRTTTexture.texture, &shaderResourceViewDesc, &g_pRTTTexture.resource);

    // Depth mapping - resource
    shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    hr = g_pd3dDevice->CreateShaderResourceView(g_pDepthTexture.texture, &shaderResourceViewDesc, &g_pDepthTexture.resource);
    hr = g_pd3dDevice->CreateShaderResourceView(g_pBloomTexture.texture, &shaderResourceViewDesc, &g_pBloomTexture.resource);
    hr = g_pd3dDevice->CreateShaderResourceView(g_pBlurTextureHorizontal.texture, &shaderResourceViewDesc, &g_pBlurTextureHorizontal.resource);
    hr = g_pd3dDevice->CreateShaderResourceView(g_pBlurTextureVertical.texture, &shaderResourceViewDesc, &g_pBlurTextureVertical.resource);

    // No MSAA - resource
    hr = g_pd3dDevice->CreateShaderResourceView(g_pNoMSAARTTTexture.texture, &shaderResourceViewDesc, &g_pNoMSAARTTTexture.resource);

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

	hr = g_GameObject.initMesh(g_pd3dDevice, g_pImmediateContext);
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
	HRESULT hr = CompileShaderFromFile(L"shaderNormal.fx", "VS", "vs_4_0", &pVSBlob);
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
    hr = CompileShaderFromFile(L"shaderNormal.fx", "RTT_VS", "vs_4_0", &pRTTVSBlob);
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


    // Set up geometry shader
    ID3DBlob* pGSBlob = nullptr;
    hr = CompileShaderFromFile(L"shaderNormal.fx", "GS", "gs_4_0", &pGSBlob);
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
    hr = CompileShaderFromFile(L"shaderNormal.fx", "GS_BILL", "gs_4_0", &pGSBillBlob);
    if (FAILED(hr))
    {
        MessageBox(nullptr, L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
        return hr;
    }
    hr = g_pd3dDevice->CreateGeometryShader(pGSBillBlob->GetBufferPointer(), pGSBillBlob->GetBufferSize(), nullptr, &g_GeometryBillboardShader);
    if (FAILED(hr))
    {
        pGSBillBlob->Release();
        return hr;
    }

    // Compile the depth geometry shader
    ID3DBlob* pDepthGSBlob = nullptr;
    hr = CompileShaderFromFile(L"shaderNormal.fx", "GS_Depth", "gs_4_0", &pDepthGSBlob);
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
	hr = CompileShaderFromFile(L"shaderNormal.fx", "PS", "ps_4_0", &pPSBlob);
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
    hr = CompileShaderFromFile(L"shaderNormal.fx", "PS_BILL", "ps_4_0", &pPSBillBlob);
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
    hr = CompileShaderFromFile(L"shaderNormal.fx", "PS_Depth", "ps_4_0", &pPSDepthBlob);
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
    hr = CompileShaderFromFile(L"shaderNormal.fx", "PS_Tint", "ps_4_0", &pPSTintBlob);
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
    hr = CompileShaderFromFile(L"shaderNormal.fx", "PS_Blur", "ps_4_0", &pPSBlurBlob);
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
    hr = CompileShaderFromFile(L"shaderNormal.fx", "RTT_PS", "ps_4_0", &pPSRTTBlob);
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
    /*for (short i = 0; i < 4; ++i)
    {
        g_ScreenQuad[i].pos.x = (g_ScreenQuad[i].pos.x / 2) - 0.5f;
        g_ScreenQuad[i].pos.y = (g_ScreenQuad[i].pos.y / 2) - 0.5f;
    }*/

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
                g_pSpriteArray[k + j * 5 + i * 25].pos = XMFLOAT3(-10.0f + 2 * i, 5.0f + 2 * j, -10.0f + 2 * k);
                g_pSpriteArray[k + j * 5 + i * 25].tex = XMFLOAT2(0.0f, 0.0f);
            }
        }
    }

    g_Spline = Spline(g_pd3dDevice);

    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(SCREEN_VERTEX) * g_numberOfSprites;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = 0;

    // Create sprite vertex buffer
    InitData.pSysMem = g_pSpriteArray;
    hr = g_pd3dDevice->CreateBuffer(&bd, &InitData, &g_pSpriteVertexBuffer);

	// Create the constant buffer
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(ConstantBuffer);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = 0;
	hr = g_pd3dDevice->CreateBuffer(&bd, nullptr, &g_pConstantBuffer);

	// Create the light constant buffer
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(LightPropertiesConstantBuffer);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = 0;
	hr = g_pd3dDevice->CreateBuffer(&bd, nullptr, &g_pLightConstantBuffer);

    // Create the billboard constant buffer
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(BillboardConstantBuffer);
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bd.CPUAccessFlags = 0;
    hr = g_pd3dDevice->CreateBuffer(&bd, nullptr, &g_pSpriteConstantBuffer);

    // Create the material constant buffer
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(MaterialPropertiesConstantBuffer);
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bd.CPUAccessFlags = 0;
    hr = g_pd3dDevice->CreateBuffer(&bd, nullptr, &g_pMaterialConstantBuffer);

    // Create the blur constant buffer
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(BlurProperties);
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bd.CPUAccessFlags = 0;
    hr = g_pd3dDevice->CreateBuffer(&bd, nullptr, &g_pBlurConstantBuffer);

	return hr;
}

// ***************************************************************************************
// InitWorld
// ***************************************************************************************
HRESULT	InitWorld(int width, int height)
{
    g_Camera = Camera(WINDOW_HEIGHT, WINDOW_WIDTH, XMFLOAT3(0.0f, 0.0f, -3.0f), XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, 1.0f, 0.0f));

    g_GameObject.setPosition({ 0.0f, 0.0f, 0.0f });

    g_pLightPos = { 0.0f, 0.0f, -2.0f, 0.0f };

	return S_OK;
}

//--------------------------------------------------------------------------------------
// Clean up the objects we've created
//--------------------------------------------------------------------------------------
void CleanupDevice()
{
    g_GameObject.cleanup();

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
    if (g_pRTTTexture.texture) g_pRTTTexture.texture->Release();
    if (g_pRTTTexture.view) g_pRTTTexture.view->Release();
    if (g_pRTTTexture.resource) g_pRTTTexture.resource->Release();
    if (g_pRTTStencilView) g_pRTTStencilView->Release();
    if (g_pScreenQuadVB) g_pScreenQuadVB->Release();
    if (g_pQuadLayout) g_pQuadLayout->Release();
    if (g_pQuadVS) g_pQuadVS->Release();
    if (g_pQuadPS) g_pQuadPS->Release();
    if (g_pBillPS) g_pBillPS->Release();
    if (g_pSpriteVertexBuffer) g_pSpriteVertexBuffer->Release();
    if (g_pSpriteLayout) g_pSpriteLayout->Release();
    if (g_GeometryBillboardShader) g_GeometryBillboardShader->Release();
    if (g_pSpriteTexture) g_pSpriteTexture->Release();
    if (g_pSpriteConstantBuffer) g_pSpriteConstantBuffer->Release();
    if (g_pDepthTexture.texture) g_pDepthTexture.texture->Release();
    if (g_pDepthTexture.view) g_pDepthTexture.view->Release();
    if (g_pDepthTexture.resource) g_pDepthTexture.resource->Release();
    if (g_pDepthGS) g_pDepthGS->Release();
    if (g_pDepthPS) g_pDepthPS->Release();
    if (g_pNoMSAARTTTexture.texture) g_pNoMSAARTTTexture.texture->Release();
    if (g_pNoMSAARTTTexture.view) g_pNoMSAARTTTexture.view->Release();
    if (g_pNoMSAARTTTexture.resource) g_pNoMSAARTTTexture.resource->Release();
    if (g_pNoMSAARTTStencilView) g_pNoMSAARTTStencilView->Release();
    if (g_pNoMSAADepthStencilTexture) g_pNoMSAADepthStencilTexture->Release();
    if (g_pTintPS) g_pTintPS->Release();
    if (g_pMaterialConstantBuffer) g_pMaterialConstantBuffer->Release();

    if (g_pBloomTexture.texture) g_pBloomTexture.texture->Release();
    if (g_pBloomTexture.view) g_pBloomTexture.view->Release();
    if (g_pBloomTexture.resource) g_pBloomTexture.resource->Release();
    if (g_pBlurTextureHorizontal.texture) g_pBlurTextureHorizontal.texture->Release();
    if (g_pBlurTextureHorizontal.view) g_pBlurTextureHorizontal.view->Release();
    if (g_pBlurTextureHorizontal.resource) g_pBlurTextureHorizontal.resource->Release();
    if (g_pBlurTextureVertical.texture) g_pBlurTextureVertical.texture->Release();
    if (g_pBlurTextureVertical.view) g_pBlurTextureVertical.view->Release();
    if (g_pBlurTextureVertical.resource) g_pBlurTextureVertical.resource->Release();

    if (g_pBlurPS) g_pBlurPS->Release();
    if (g_pBlurConstantBuffer) g_pBlurConstantBuffer->Release();

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
        g_Camera.ChangeActive();
        if (g_Camera.GetActive())
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
    if (g_Camera.GetActive())
    {
        const float speed = 5.0f;
        if (GetAsyncKeyState('W'))
        {
            g_Camera.CameraTranslate({ 0.0f, 0.0f, speed * deltaTime }, g_Camera.GetPitch(), g_Camera.GetYaw());
        }
        if (GetAsyncKeyState('A'))
        {
            g_Camera.CameraTranslate({ -speed * deltaTime, 0.0f, 0.0f }, g_Camera.GetPitch(), g_Camera.GetYaw());
        }
        if (GetAsyncKeyState('S'))
        {
            g_Camera.CameraTranslate({ 0.0f, 0.0f, -speed * deltaTime }, g_Camera.GetPitch(), g_Camera.GetYaw());
        }
        if (GetAsyncKeyState('D'))
        {
            g_Camera.CameraTranslate({ speed * deltaTime, 0.0f, 0.0f }, g_Camera.GetPitch(), g_Camera.GetYaw());
        }
        if (GetAsyncKeyState('Q'))
        {
            g_Camera.CameraTranslate({ 0.0f, speed * deltaTime, 0.0f }, 0, 0);
        }
        if (GetAsyncKeyState('E'))
        {
            g_Camera.CameraTranslate({ 0.0f, -speed * deltaTime, 0.0f }, 0, 0);
        }
    }
}

void setupLightForRender()
{
    LightPropertiesConstantBuffer lightProperties;

    lightProperties.EyePosition = g_Camera.GetEye();
    lightProperties.Lights[0].Enabled = static_cast<int>(true);
    lightProperties.Lights[0].LightType = PointLight;
    lightProperties.Lights[0].Color = XMFLOAT4(Colors::White);
    lightProperties.Lights[0].SpotAngle = XMConvertToRadians(45.0f);
    lightProperties.Lights[0].ConstantAttenuation = 0.5;
    lightProperties.Lights[0].LinearAttenuation = 0.5;
    lightProperties.Lights[0].QuadraticAttenuation = 0.5;
    lightProperties.Lights[0].Position = { g_pLightPos.x + guiLightX, g_pLightPos.y + guiLightY, g_pLightPos.z + guiLightZ, 0.0f };
    XMVECTOR LightDirection = XMVectorSet(-g_pLightPos.x + guiLightX, -g_pLightPos.y + guiLightY, -g_pLightPos.z + guiLightZ, 0.0f);
    LightDirection = XMVector3Normalize(LightDirection);
    XMStoreFloat4(&lightProperties.Lights[0].Direction, LightDirection);

    g_pImmediateContext->UpdateSubresource(g_pLightConstantBuffer, 0, nullptr, &lightProperties, 0, 0);

    // Set up billboard
    BillboardConstantBuffer billboardProperties;
    billboardProperties.EyePos = g_Camera.GetEye();
    billboardProperties.UpVector = g_Camera.GetUp();
    g_pImmediateContext->UpdateSubresource(g_pSpriteConstantBuffer, 0, nullptr, &billboardProperties, 0, 0);

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
    blurProps.mouseChange = g_Camera.GetChange();
    blurProps.Padding = 0;
    g_pImmediateContext->UpdateSubresource(g_pBlurConstantBuffer, 0, nullptr, &blurProps, 0, 0);
    g_pImmediateContext->PSSetConstantBuffers(4, 1, &g_pBlurConstantBuffer);
}

void DrawScene(ConstantBuffer* cb)
{
    // Rendering
    XMMATRIX mGO = XMLoadFloat4x4(g_GameObject.getTransform());
    const XMVECTOR pos = XMLoadFloat4(&g_pLightPos);
    cb->mWorld = XMMatrixTranspose(mGO);
    g_pImmediateContext->UpdateSubresource(g_pConstantBuffer, 0, nullptr, cb, 0, 0);
    g_GameObject.draw(g_pImmediateContext);
}

void DrawSceneSprites()
{
    UINT stride = sizeof(SCREEN_VERTEX);
    UINT offset = 0;
    ID3D11Buffer* pSpriteBuffers[1] = { g_pSpriteVertexBuffer };
    g_pImmediateContext->IASetVertexBuffers(0, 1, pSpriteBuffers, &stride, &offset);
    g_pImmediateContext->IASetInputLayout(g_pQuadLayout);
    g_pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);

    g_pImmediateContext->PSSetShaderResources(0, 1, &g_pSpriteTexture);
    g_pImmediateContext->GSSetConstantBuffers(3, 1, &g_pSpriteConstantBuffer);
    g_pImmediateContext->Draw(g_numberOfSprites, 0);
}

void Bloom(ConstantBuffer* cb)
{
    /***********************************************
    MARKING SCHEME: Advanced graphics techniques
    DESCRIPTION: Gaussian blur, Motion blur
    ***********************************************/

    g_pImmediateContext->OMSetRenderTargets(1, &g_pBloomTexture.view, g_pNoMSAARTTStencilView);
    g_pImmediateContext->ClearRenderTargetView(g_pBloomTexture.view, Colors::MidnightBlue);
    g_pImmediateContext->ClearDepthStencilView(g_pNoMSAARTTStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

    // Draw scene to target
    g_pImmediateContext->VSSetShader(g_pVertexShader, nullptr, 0);
    g_pImmediateContext->GSSetShader(g_GeometryShader, nullptr, 0);
    g_pImmediateContext->PSSetShader(g_pPixelShader, nullptr, 0);
    g_pImmediateContext->IASetInputLayout(g_pVertexLayout);

    DrawScene(cb);

    g_pImmediateContext->VSSetShader(g_pQuadVS, nullptr, 0);
    g_pImmediateContext->GSSetShader(g_GeometryBillboardShader, nullptr, 0);
    g_pImmediateContext->PSSetShader(g_pBillPS, nullptr, 0);

    DrawSceneSprites();

    ////////////////////////////////////////////////////////////////////////////////////////////////////

    g_pImmediateContext->OMSetRenderTargets(1, &g_pBlurTextureHorizontal.view, g_pNoMSAARTTStencilView);
    g_pImmediateContext->ClearRenderTargetView(g_pBlurTextureHorizontal.view, Colors::Black);
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
    g_pImmediateContext->PSSetShaderResources(0, 1, &g_pBloomTexture.resource);

    g_pImmediateContext->Draw(4, 0);

    ////////////////////////////////////////////////////////////////////////////////////////////////////

    g_pImmediateContext->OMSetRenderTargets(1, &g_pBlurTextureVertical.view, g_pNoMSAARTTStencilView);
    g_pImmediateContext->ClearRenderTargetView(g_pBlurTextureVertical.view, Colors::Black);
    g_pImmediateContext->ClearDepthStencilView(g_pNoMSAARTTStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

    BlurProperties blurProps;
    blurProps.isHorizontal = 0;
    blurProps.mouseChange = g_Camera.GetChange();
    blurProps.Padding = 0;
    g_pImmediateContext->UpdateSubresource(g_pBlurConstantBuffer, 0, nullptr, &blurProps, 0, 0);

    // Render to quad for 2nd (vertical) pass
    stride = sizeof(SCREEN_VERTEX);
    offset = 0;
    pBuffers[0] = { g_pScreenQuadVB };
    g_pImmediateContext->IASetVertexBuffers(0, 1, pBuffers, &stride, &offset);

    g_pImmediateContext->PSSetShaderResources(0, 1, &g_pBlurTextureHorizontal.resource);

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
    g_pImmediateContext->OMSetRenderTargets(1, &g_pRTTTexture.view, g_pDepthStencilView);
    g_pImmediateContext->ClearRenderTargetView(g_pRTTTexture.view, Colors::MidnightBlue);
    g_pImmediateContext->ClearDepthStencilView(g_pDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

    // Draw scene to RTT target
    g_pImmediateContext->VSSetShader(g_pVertexShader, nullptr, 0);
    g_pImmediateContext->GSSetShader(g_GeometryShader, nullptr, 0);
    g_pImmediateContext->PSSetShader(g_pPixelShader, nullptr, 0);
    g_pImmediateContext->IASetInputLayout(g_pVertexLayout);

    DrawScene(cb);

    g_pImmediateContext->VSSetShader(g_pQuadVS, nullptr, 0);
    g_pImmediateContext->GSSetShader(g_GeometryBillboardShader, nullptr, 0);
    g_pImmediateContext->PSSetShader(g_pBillPS, nullptr, 0);

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
        g_pImmediateContext->PSSetShaderResources(0, 1, &g_pDepthTexture.resource);
    }
    else
    {
        /***********************************************
        MARKING SCHEME: Advanced graphics techniques
        DESCRIPTION: MSAA
        ***********************************************/
        g_pImmediateContext->ResolveSubresource(g_pNoMSAARTTTexture.texture, 0, g_pRTTTexture.texture, 
                                                0, DXGI_FORMAT_R32G32B32A32_FLOAT);
        g_pImmediateContext->PSSetShaderResources(0, 1, &g_pNoMSAARTTTexture.resource);
        if (guiMotionBlur)
        {
            g_pImmediateContext->PSSetShaderResources(0, 1, &g_pBlurTextureVertical.resource);
        }
    }
    
    g_pImmediateContext->Draw(4, 0);
    ID3D11ShaderResourceView* nullSRV = { nullptr };
    g_pImmediateContext->PSSetShaderResources(0, 1, &nullSRV);
    g_pImmediateContext->PSSetShaderResources(3, 1, &nullSRV);
}

void DepthMap(ConstantBuffer* cb)
{
    g_pImmediateContext->OMSetRenderTargets(1, &g_pDepthTexture.view, g_pNoMSAARTTStencilView);
    g_pImmediateContext->ClearRenderTargetView(g_pDepthTexture.view, Colors::Black);
    g_pImmediateContext->ClearDepthStencilView(g_pNoMSAARTTStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

    g_pImmediateContext->VSSetShader(g_pQuadVS, nullptr, 0);
    g_pImmediateContext->GSSetShader(g_pDepthGS, nullptr, 0);
    g_pImmediateContext->PSSetShader(g_pDepthPS, nullptr, 0);

    g_pImmediateContext->IASetInputLayout(g_pQuadLayout);

    DrawScene(cb);

    g_pImmediateContext->GSSetShader(g_GeometryBillboardShader, nullptr, 0);

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

    // Update the cube transform, material etc.
    float tempT = (guiRotation ? t : 0);
    g_GameObject.update(tempT, g_pImmediateContext);
    g_Camera.Update(g_hWnd);
    HandlePerFrameInput(t);

    // Get the game object world transform
    XMFLOAT4X4 v = g_Camera.GetView();
    XMFLOAT4X4 p = g_Camera.GetProjection();

    // Store this and the view / projection in a constant buffer for the vertex shader to use
    ConstantBuffer cb1;
    cb1.vOutputColor = XMFLOAT4(0, 0, 0, 0);
    cb1.mView = XMMatrixTranspose(XMLoadFloat4x4(&v));
    cb1.mProjection = XMMatrixTranspose(XMLoadFloat4x4(&p));
    g_pImmediateContext->UpdateSubresource(g_pConstantBuffer, 0, nullptr, &cb1, 0, 0);

    setupLightForRender();

    g_pImmediateContext->GSSetConstantBuffers(0, 1, &g_pConstantBuffer);
    g_pImmediateContext->PSSetConstantBuffers(2, 1, &g_pLightConstantBuffer);
    g_pImmediateContext->GSSetConstantBuffers(2, 1, &g_pLightConstantBuffer);

    // Draw functions
    if (guiMotionBlur)
    {
        Bloom(&cb1);
    }
    DepthMap(&cb1);

    g_pImmediateContext->OMSetRenderTargets(1, &g_pRenderTargetView, g_pDepthStencilView);
    // Clear the back buffer
    g_pImmediateContext->ClearRenderTargetView(g_pRenderTargetView, Colors::MidnightBlue);
    g_pImmediateContext->ClearDepthStencilView(g_pDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

    g_pImmediateContext->VSSetShader(g_pVertexShader, nullptr, 0);
    g_pImmediateContext->GSSetShader(g_GeometryShader, nullptr, 0);
    g_pImmediateContext->PSSetShader(g_pPixelShader, nullptr, 0);

    g_pImmediateContext->IASetInputLayout(g_pVertexLayout);

    DrawScene(&cb1);

    g_pImmediateContext->VSSetShader(g_pQuadVS, nullptr, 0);
    g_pImmediateContext->GSSetShader(g_GeometryBillboardShader, nullptr, 0);
    g_pImmediateContext->PSSetShader(g_pBillPS, nullptr, 0);

    DrawSceneSprites();

    RenderScreenQuad(&cb1);

    // Spline
    g_Spline.Render(g_pImmediateContext, g_pQuadLayout);

    // ImGui
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    // The window
    ImGui::Begin("Options");
    static const char* items[]{ "Diffuse", "Normals", "Parallax", "Parallax Occlusion", "Self-Shadowing POM"};
    ImGui::ListBox("Shading", &materialSelection, items, ARRAYSIZE(items));
    static const char* items2[]{ "Default", "Depth Render", "Invert Colours" };
    ImGui::ListBox("Render Mode", &guiSelection, items2, ARRAYSIZE(items2));
    ImGui::SliderFloat("Light X Pos", &guiLightX, -3.0f, 3.0f);
    ImGui::SliderFloat("Light Y Pos", &guiLightY, -3.0f, 3.0f);
    ImGui::SliderFloat("Light Z Pos", &guiLightZ, -3.0f, 3.0f);
    ImGui::Checkbox("Enable Motion Blur", &guiMotionBlur);
    ImGui::Checkbox("Enable Rotation", &guiRotation);
    ImGui::End();

    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    // Present our back buffer to our front buffer
    g_pSwapChain->Present(0, 0);
}