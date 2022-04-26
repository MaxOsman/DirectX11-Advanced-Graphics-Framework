#pragma once
#include "Quaternion.h"
#include "DrawableGameObject.h"
#include <vector>

class Bone : public DrawableGameObject
{
public:
	Bone();
	~Bone();

	HRESULT	initMesh(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pContext);

	void draw(ID3D11DeviceContext* pContext);
	void draw(ID3D11DeviceContext* pContext, ID3D11ShaderResourceView* texture);

	void boneUpdate(ID3D11DeviceContext* pContext);

private:
	Quaternion m_orientation;
	std::vector<Bone*> m_childBones;
};