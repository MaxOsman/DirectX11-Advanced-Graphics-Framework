#pragma once

#include "DrawableGameObject.h"
#include <vector>

#define TERRAIN_TEX_SIZE 5

class TerrainGameObject : public DrawableGameObject
{
public:
	TerrainGameObject();
	~TerrainGameObject();

	HRESULT	initMesh(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pContext) { return HRESULT(); }
	HRESULT	initMesh(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pContext, int type);

	void draw(ID3D11DeviceContext* pContext);
	void draw(ID3D11DeviceContext* pContext, ID3D11ShaderResourceView* texture);

	void SetHeight(float h) { height = h; }

private:
	void LoadHeightMap();
	void FaultAlgorithm();
	void Deposit(int x, int y);
	void ParticleDeposition();
	void DiamondSquareAlgorithm();
	void DiamondStage(int sideLength);
	void SquareStage(int sideLength);
	void Average(int x, int y, int sideLength);

	ID3D11ShaderResourceView* m_pTerrainTextures[TERRAIN_TEX_SIZE];
	ID3D11ShaderResourceView* m_pHeightTexture;
	ID3D11ShaderResourceView* m_pNormalTexture;

	float height = 10.0f;
	int range;
	float** heightArray;
};