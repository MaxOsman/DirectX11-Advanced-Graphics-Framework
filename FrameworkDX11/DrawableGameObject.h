#pragma once

#include "DDSTextureLoader.h"
//#include <iostream>
#include "structures.h"

struct SimpleVertex
{
	XMFLOAT3 Pos;
	XMFLOAT3 Normal;
	XMFLOAT2 TexCoord;
	XMFLOAT3 Tangent;
	XMFLOAT3 BiTangent;
};

class DrawableGameObject
{
public:
	DrawableGameObject();
	~DrawableGameObject();

	void cleanup();

	virtual HRESULT						initMesh(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pContext) = 0;
	void								update(float t, ID3D11DeviceContext* pContext);
	void								update(ID3D11DeviceContext* pContext);
	virtual void						draw(ID3D11DeviceContext* pContext) = 0;
	virtual void						draw(ID3D11DeviceContext* pContext, ID3D11ShaderResourceView* texture) = 0;
	ID3D11Buffer*						getVertexBuffer() { return m_pVertexBuffer; }
	ID3D11Buffer*						getIndexBuffer() { return m_pIndexBuffer; }
	ID3D11ShaderResourceView**			getTextureResourceView() { return &m_pTextureResourceView; 	}
	XMFLOAT4X4*							getTransform() { return &m_World; }
	XMFLOAT3							getPosition() { return m_position; }
	ID3D11SamplerState**				getTextureSamplerState() { return &m_pSamplerLinear; }
	void								setPosition(XMFLOAT3 position);
	void								CalculateModelVectors(SimpleVertex* vertices, int vertexCount);
	void								CalculateTangentBinormalLH(SimpleVertex v0, SimpleVertex v1, SimpleVertex v2, XMFLOAT3& normal, XMFLOAT3& tangent, XMFLOAT3& binormal);
	void								CalculateTangentBinormalRH(SimpleVertex v0, SimpleVertex v1, SimpleVertex v2, XMFLOAT3& normal, XMFLOAT3& tangent, XMFLOAT3& binormal);
	ID3D11SamplerState*					getSampler() { return m_pSamplerLinear; }
	void								setScale(XMFLOAT3 scale) { m_scale = scale; }

protected:
	
	XMFLOAT4X4							m_World;
	ID3D11Buffer*						m_pVertexBuffer;
	ID3D11Buffer*						m_pIndexBuffer;
	ID3D11ShaderResourceView*			m_pTextureResourceView;
	ID3D11ShaderResourceView*			m_pNormalTexture;
	ID3D11ShaderResourceView*			m_pParallaxTexture;
	ID3D11SamplerState *				m_pSamplerLinear;
	XMFLOAT3							m_position;
	XMFLOAT3							m_scale = { 1.0f, 1.0f, 1.0f };
};