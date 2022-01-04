#pragma once

#include <windows.h>
#include <windowsx.h>
#include <d3d11_1.h>
#include <d3dcompiler.h>
#include <directxmath.h>
#include <directxcolors.h>
#include <DirectXCollision.h>
#include "DDSTextureLoader.h"
#include "resource.h"
#include "Math.h"

using namespace DirectX;

class Camera
{
private:
	XMFLOAT4X4 _view;
	XMFLOAT4X4 _projection;

	RECT rc;
	POINT point;

	XMFLOAT3 _eye;
	XMFLOAT3 _at;
	XMFLOAT3 _up;

	float pitch = 0.0f;
	float yaw = 0.0f;
	float rotationSpeed = 0.004f;

	bool isActive = false;

public:
	Camera() {}
	Camera(int windowHeight, int windowWidth, XMFLOAT3 eye, XMFLOAT3 at, XMFLOAT3 up);

	void Update(HWND hWnd);

	XMFLOAT4X4 GetView() { return _view; }
	XMFLOAT4X4 GetProjection() { return _projection; }
	XMFLOAT4 GetEye() { return XMFLOAT4(_eye.x, _eye.y, _eye.z, 1.0f); }
	XMFLOAT3 GetAt() { return _at; }
	XMFLOAT4 GetUp() { return XMFLOAT4( _up.x, _up.y, _up.z, 0.0f ); }
	float GetYaw() { return yaw; }
	float GetPitch() { return pitch; }
	
	void Reshape(UINT windowWidth, UINT windowHeight, FLOAT nearDepth, FLOAT farDepth);
	void SetEye(XMFLOAT3 e) { _eye = e; }
	void CameraTranslate(XMFLOAT3 d, float pitch, float yaw);
	void Rotate(float dx, float dy);

	XMMATRIX GetMatrix1st();
	float WrapAngle(float a);
	float Clamp(float value, float upper, float lower);
	void ChangeActive() { isActive = !isActive; }
	bool GetActive() { return isActive; }
};