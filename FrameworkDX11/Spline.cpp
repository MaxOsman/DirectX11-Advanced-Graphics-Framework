#include "Spline.h"

Spline::Spline(ID3D11Device* g_pd3dDevice)
{
	for (size_t i = 0; i < POINT_COUNT; ++i)
	{
		m_Points[i] = { {0, 0, 0}, {0, 0} };
	}

	D3D11_BUFFER_DESC bd = {};
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(SCREEN_VERTEX) * POINT_COUNT;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;

	// Create vertex buffer
	D3D11_SUBRESOURCE_DATA InitData = {};
	InitData.pSysMem = m_Points;
	HRESULT hr = g_pd3dDevice->CreateBuffer(&bd, &InitData, &g_pSplineVB);
	if (FAILED(hr))
	{

	}
}

Spline::~Spline()
{
	if (g_pSplineVB) g_pSplineVB->Release();
}

void Spline::Render(ID3D11DeviceContext* g_pImmediateContext, ID3D11InputLayout* g_pQuadLayout)
{
	g_pImmediateContext->IASetInputLayout(g_pQuadLayout);
	g_pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP);
}
