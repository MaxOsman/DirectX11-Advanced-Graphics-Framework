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
        sd.SampleDesc.Count = 1;
        sd.SampleDesc.Quality = 0;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.BufferCount = 1;

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
        sd.SampleDesc.Count = 1;
        sd.SampleDesc.Quality = 0;
        sd.Windowed = TRUE;

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

    // Create depth stencil texture
    D3D11_TEXTURE2D_DESC descDepth = {};
    descDepth.Width = width;
    descDepth.Height = height;
    descDepth.MipLevels = 1;
    descDepth.ArraySize = 1;
    descDepth.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    descDepth.SampleDesc.Count = 1;
    descDepth.SampleDesc.Quality = 0;
    descDepth.Usage = D3D11_USAGE_DEFAULT;
    descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    descDepth.CPUAccessFlags = 0;
    descDepth.MiscFlags = 0;
    hr = g_pd3dDevice->CreateTexture2D( &descDepth, nullptr, &g_pDepthStencil );
    if( FAILED( hr ) )
        return hr;

    // Create the depth stencil view
    D3D11_DEPTH_STENCIL_VIEW_DESC descDSV = {};
    descDSV.Format = descDepth.Format;
    descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    descDSV.Texture2D.MipSlice = 0;
    hr = g_pd3dDevice->CreateDepthStencilView( g_pDepthStencil, &descDSV, &g_pDepthStencilView );
    if( FAILED( hr ) )
        return hr;

    g_pImmediateContext->OMSetRenderTargets( 1, &g_pRenderTargetView, g_pDepthStencilView );

    // Week 6 - Render to texture
    D3D11_TEXTURE2D_DESC textureDesc;
    ZeroMemory(&textureDesc, sizeof(textureDesc));

    textureDesc.Width = width;
    textureDesc.Height = height;
    textureDesc.MipLevels = 1;
    textureDesc.ArraySize = 1;
    textureDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.Usage = D3D11_USAGE_DEFAULT;
    textureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    textureDesc.CPUAccessFlags = 0;
    textureDesc.MiscFlags = 0;

    g_pd3dDevice->CreateTexture2D(&textureDesc, NULL, &g_pRTTRenderTargetTexture);
    if (FAILED(hr))
        return hr;

    D3D11_RENDER_TARGET_VIEW_DESC renderTargetViewDesc;

    D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc;

    g_pImmediateContext->OMSetRenderTargets(1, &g_pRTTRenderTargetView, g_pRTTStencilView);

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
		MessageBox(nullptr,
			L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
		return hr;
	}

	// Create the vertex shader
	hr = g_pd3dDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, &g_pVertexShader);
	if (FAILED(hr))
	{
		pVSBlob->Release();
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

	// Set the input layout
	g_pImmediateContext->IASetInputLayout(g_pVertexLayout);

	// Compile the pixel shader
	ID3DBlob* pPSBlob = nullptr;
	hr = CompileShaderFromFile(L"shaderNormal.fx", "PS", "ps_4_0", &pPSBlob);
	if (FAILED(hr))
	{
		MessageBox(nullptr,
			L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
		return hr;
	}

	// Create the pixel shader
	hr = g_pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &g_pPixelShader);
	pPSBlob->Release();
	if (FAILED(hr))
		return hr;

	// Create the constant buffer
    D3D11_BUFFER_DESC bd = {};
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(SimpleVertex) * 24;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = 0;
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(WORD) * 36;        // 36 vertices needed for 12 triangles in a triangle list
    bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    bd.CPUAccessFlags = 0;
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(ConstantBuffer);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = 0;
	hr = g_pd3dDevice->CreateBuffer(&bd, nullptr, &g_pConstantBuffer);
	if (FAILED(hr))
		return hr;

	// Create the light constant buffer
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(LightPropertiesConstantBuffer);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = 0;
	hr = g_pd3dDevice->CreateBuffer(&bd, nullptr, &g_pLightConstantBuffer);
	if (FAILED(hr))
		return hr;

	return hr;
}

// ***************************************************************************************
// InitWorld
// ***************************************************************************************
HRESULT	InitWorld(int width, int height)
{
    g_Camera = Camera(WINDOW_HEIGHT, WINDOW_WIDTH, XMFLOAT3(0.0f, 0.0f, -3.0f), XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, 1.0f, 0.0f));

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

    if (g_pLightConstantBuffer)
        g_pLightConstantBuffer->Release();
    if (g_pVertexLayout) g_pVertexLayout->Release();
    if( g_pConstantBuffer ) g_pConstantBuffer->Release();
    if( g_pVertexShader ) g_pVertexShader->Release();
    if( g_pPixelShader ) g_pPixelShader->Release();
    if( g_pDepthStencil ) g_pDepthStencil->Release();
    if( g_pDepthStencilView ) g_pDepthStencilView->Release();
    if( g_pRenderTargetView ) g_pRenderTargetView->Release();
    if( g_pSwapChain1 ) g_pSwapChain1->Release();
    if( g_pSwapChain ) g_pSwapChain->Release();
    if( g_pImmediateContext1 ) g_pImmediateContext1->Release();
    if( g_pImmediateContext ) g_pImmediateContext->Release();
    if (g_pRTTRenderTargetTexture) g_pRTTRenderTargetTexture->Release();
    if (g_pRTTRenderTargetView) g_pRTTRenderTargetView->Release();
    if (g_pRTTShaderResourceView) g_pRTTShaderResourceView->Release();
    if (g_pRTTStencilView) g_pRTTStencilView->Release();
    if (g_pScreenQuadVB) g_pScreenQuadVB->Release();
    if (g_pQuadLayout) g_pQuadLayout->Release();
    if (g_pQuadVS) g_pQuadVS->Release();
    if (g_pQuadPS) g_pQuadPS->Release();

    ID3D11Debug* debugDevice = nullptr;
    g_pd3dDevice->QueryInterface(__uuidof(ID3D11Debug), reinterpret_cast<void**>(&debugDevice));

    if (g_pd3dDevice1) g_pd3dDevice1->Release();
    if (g_pd3dDevice) g_pd3dDevice->Release();

    // handy for finding dx memory leaks
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

void setupLightForRender()
{
    Light light;
    light.Enabled = static_cast<int>(true);
    light.LightType = PointLight;
    light.Color = XMFLOAT4(Colors::White);
    light.SpotAngle = XMConvertToRadians(45.0f);
    light.ConstantAttenuation = 1.0f;
    light.LinearAttenuation = 1;
    light.QuadraticAttenuation = 1;

    // set up the light
    XMFLOAT4 LightPosition = { 0.0f, 0.0f, -3.0f, 1.0f };
    light.Position = LightPosition;
    XMVECTOR LightDirection = XMVectorSet(-LightPosition.x, -LightPosition.y, -LightPosition.z, 0.0f);
    LightDirection = XMVector3Normalize(LightDirection);
    XMStoreFloat4(&light.Direction, LightDirection);

    LightPropertiesConstantBuffer lightProperties;
    lightProperties.EyePosition = { g_Camera.GetEye().x, g_Camera.GetEye().y, g_Camera.GetEye().z, 1.0f };
    lightProperties.Lights[0] = light;
    g_pImmediateContext->UpdateSubresource(g_pLightConstantBuffer, 0, nullptr, &lightProperties, 0, 0);
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

//--------------------------------------------------------------------------------------
// Render a frame
//--------------------------------------------------------------------------------------
void Render()
{
    float t = calculateDeltaTime(); // capped at 60 fps
    if (t == 0.0f)
        return;

    // Clear the back buffer
    g_pImmediateContext->ClearRenderTargetView(g_pRenderTargetView, Colors::MidnightBlue);

    // Clear the depth buffer to 1.0 (max depth)
    g_pImmediateContext->ClearDepthStencilView(g_pDepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);

    // Update the cube transform, material etc. 
    g_GameObject.update(t, g_pImmediateContext);

    g_Camera.Update(g_hWnd);
    HandlePerFrameInput(t);

    // get the game object world transform
    XMMATRIX mGO = XMLoadFloat4x4(g_GameObject.getTransform());

    // store this and the view / projection in a constant buffer for the vertex shader to use
    ConstantBuffer cb1;
    cb1.mWorld = XMMatrixTranspose(mGO);
    XMFLOAT4X4 v = g_Camera.GetView();
    XMFLOAT4X4 p = g_Camera.GetProjection();
    cb1.mView = XMMatrixTranspose(XMLoadFloat4x4(&v));
    cb1.mProjection = XMMatrixTranspose(XMLoadFloat4x4(&p));
    cb1.vOutputColor = XMFLOAT4(0, 0, 0, 0);
    g_pImmediateContext->UpdateSubresource(g_pConstantBuffer, 0, nullptr, &cb1, 0, 0);

    setupLightForRender();

    // Render the cube
    g_pImmediateContext->VSSetShader(g_pVertexShader, nullptr, 0);
    g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pConstantBuffer);

    g_pImmediateContext->PSSetShader(g_pPixelShader, nullptr, 0);
    g_pImmediateContext->PSSetConstantBuffers(2, 1, &g_pLightConstantBuffer);
    ID3D11Buffer* materialCB = g_GameObject.getMaterialConstantBuffer();
    g_pImmediateContext->PSSetConstantBuffers(1, 1, &materialCB);

    g_GameObject.draw(g_pImmediateContext);

    // ImGui
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    // The window
    //ImGui::Begin("Test");
    //ImGui::End();

    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    // Present our back buffer to our front buffer
    g_pSwapChain->Present(0, 0);
}