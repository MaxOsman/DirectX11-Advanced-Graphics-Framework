#pragma once

#include "DrawableGameObject.h"

class CubeGameObject : public DrawableGameObject
{
public:
	CubeGameObject();
	~CubeGameObject();

	HRESULT	initMesh(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pContext);

	void draw(ID3D11DeviceContext* pContext);
	void draw(ID3D11DeviceContext* pContext, ID3D11ShaderResourceView* texture);

private:
	
};