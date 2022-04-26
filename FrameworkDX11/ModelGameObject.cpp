#include "ModelGameObject.h"

ModelGameObject::ModelGameObject(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pContext)
{
	m_pRootBone = new Bone();
	m_pRootBone->setPosition({ 12, 0.0f, 12 });
}

ModelGameObject::~ModelGameObject()
{
	m_pRootBone->cleanup();
	m_pRootBone = nullptr;
	delete m_pRootBone;
}

void ModelGameObject::Draw(ID3D11DeviceContext* pContext)
{
	m_pRootBone->draw(pContext);
}

void ModelGameObject::Update(float t, ID3D11DeviceContext* pContext)
{
	m_pRootBone->update(t, pContext);
}

HRESULT ModelGameObject::InitMesh(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pContext)
{
	return m_pRootBone->initMesh(pd3dDevice, pContext);
}