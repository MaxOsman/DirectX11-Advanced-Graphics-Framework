#include "Camera.h"

Camera::Camera(int windowHeight, int windowWidth, XMFLOAT3 eye, XMFLOAT3 at, XMFLOAT3 up)
{
    _eye = eye;
    _at = at;
    _up = up;

    // Initialize the view matrix
    XMVECTOR Eye = XMVectorSet(_eye.x, _eye.y, _eye.z, 0.0f);
    XMVECTOR At = XMVectorSet(_at.x, _at.y, _at.z, 0.0f);
    XMVECTOR Up = XMVectorSet(_up.x, _up.y, _up.z, 0.0f);
    XMStoreFloat4x4(&_view, XMMatrixLookAtLH(Eye, At, Up));

    // Initialize the projection matrix
    XMStoreFloat4x4(&_projection, XMMatrixPerspectiveFovLH(XM_PIDIV2, windowHeight * XM_PI / (FLOAT)windowWidth, 0.01f, 100.0f));
}

void Camera::Update(HWND hWnd)
{
    if (isActive)
    {
        // Get screen coords
        GetClientRect(hWnd, &rc);
        MapWindowPoints(hWnd, nullptr, reinterpret_cast<POINT*>(&rc), 2);

        // Check position
        GetCursorPos(&point);
        /*g_Debug.Print(float(point.x - (0.5 * rc.right + 0.5 * rc.left)));
        g_Debug.Print(float(point.y - (0.5 * rc.bottom + 0.5 * rc.top)));*/
        _mouseChange = { float(point.x - (0.5 * rc.right + 0.5 * rc.left)), float(point.y - (0.5 * rc.bottom + 0.5 * rc.top)) };
        POINT newPoint = { point.x - (0.5 * rc.right + 0.5 * rc.left), point.y - (0.5 * rc.bottom + 0.5 * rc.top) };

        // Do player rotation
        Rotate(newPoint.x, newPoint.y);

        // Reset position
        SetCursorPos(0.5 * rc.right + 0.5 * rc.left, 0.5 * rc.bottom + 0.5 * rc.top);

        XMStoreFloat4x4(&_view, GetMatrix1st());

        // Update up vector
        XMFLOAT3 tempUp = XMFLOAT3(0.0f, 1.0f, 0.0f);
        XMStoreFloat3(&tempUp, XMVector3Transform(XMLoadFloat3(&tempUp), XMMatrixRotationRollPitchYaw(pitch, yaw, 0.0f)));
        _up = tempUp;
    }
}

void Camera::Reshape(UINT windowWidth, UINT windowHeight, FLOAT nearDepth, FLOAT farDepth)
{
    XMStoreFloat4x4(&_projection, XMMatrixPerspectiveFovLH(XM_PIDIV2, windowWidth * XM_PI / (FLOAT)windowHeight, nearDepth, farDepth));
}

void Camera::CameraTranslate(XMFLOAT3 d, float pitch, float yaw)
{
    XMStoreFloat3(&d, XMVector3Transform(XMLoadFloat3(&d), XMMatrixRotationRollPitchYaw(pitch, yaw, 0.0f)));
    _eye = { _eye.x + d.x, _eye.y + d.y, _eye.z + d.z };
}

void Camera::Rotate(float dx, float dy)
{
    yaw = WrapAngle(yaw + (dx * rotationSpeed));
    pitch = Clamp(pitch + (dy * rotationSpeed), 0.995f * XM_PI / 2.0f, 0.995f * -XM_PI / 2.0f);
}

XMMATRIX Camera::GetMatrix1st()
{
    const XMVECTOR lookVector = XMVector3Transform(XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), XMMatrixRotationRollPitchYaw(pitch, yaw, 0.0f));

    XMFLOAT3 tempMatrix = { _eye.x, _eye.y, _eye.z };
    const XMVECTOR camPosition = XMLoadFloat3(&tempMatrix);
    const XMVECTOR camTarget = camPosition + lookVector;

    return XMMatrixLookAtLH(camPosition, camTarget, XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));
}

float Camera::WrapAngle(float ang)
{
    ang = fmod(ang + 180, 360);
    if (ang < 0)
        ang += 360;

    ang -= 180;
    return ang;
}

float Camera::Clamp(float x, float upper, float lower)
{
    return min(upper, max(x, lower));
}