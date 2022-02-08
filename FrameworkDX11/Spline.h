#pragma once
#include "structures.h"
#include <DirectXMath.h>
#include <d3d11_1.h>

#define POINT_COUNT 11

class Spline
{
public:
	Spline(ID3D11Device* g_pd3dDevice);
	~Spline();

	void Render(ID3D11DeviceContext* g_pImmediateContext, ID3D11InputLayout* g_pQuadLayout);

private:
	SCREEN_VERTEX m_Points[POINT_COUNT];

	ID3D11Buffer* g_pSplineVB = nullptr;
};