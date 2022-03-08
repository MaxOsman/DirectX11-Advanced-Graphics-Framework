#pragma once

#include "DrawableGameObject.h"
#include <vector>

#define TERRAIN_TEX_SIZE 5

class TerrainGameObject : public DrawableGameObject
{
public:
	TerrainGameObject();
	~TerrainGameObject();

	HRESULT	initMesh(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pContext);

	void draw(ID3D11DeviceContext* pContext);
	void draw(ID3D11DeviceContext* pContext, ID3D11ShaderResourceView* texture);

private:
	vector<float>* LoadHeightMap();

	ID3D11ShaderResourceView* m_pTerrainTextures[TERRAIN_TEX_SIZE];
	ID3D11ShaderResourceView* m_pHeightTexture;
};