#pragma once
#include "Bone.h"

class ModelGameObject
{
public:
	ModelGameObject() {}
	ModelGameObject(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pContext);
	~ModelGameObject();

	void Draw(ID3D11DeviceContext* pContext);
	void Update(float t, ID3D11DeviceContext* pContext);
	void Update(ID3D11DeviceContext* pContext);

	XMFLOAT4X4* GetTransform() { return m_pRootBone->getTransform(); }
	HRESULT	InitMesh(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pContext);

private:
	Bone* m_pRootBone;
};