#include "ZNCamera.h"
#include "ZNInputDef.h"
#include <math.h>

using namespace ZNFramework;

ZNCamera::ZNCamera()
	: viewMatrix(ZNMatrix4())
	, projectionMatrix(ZNMatrix4())
	, position(ZNVector3(0.0f, 0.0f, -5.0f))
	, forward(ZNVector3(0.0f, 0.0f, 1.0f))
	, right(ZNVector3(1.0f, 0.0f, 0.0f))
	, up(ZNVector3(0.0f, 1.0f, 0.0f))
	, pitch(0.0f)
	, yaw(0.0f)
	, moveSpeed(5.0f)
	, autoUpdateView(true)
	, isDragging(false)
	, lastMouseX(0)
	, lastMouseY(0)
{
	UpdateViewMatrix();
}

void ZNFramework::ZNCamera::SetView(const ZNVector3& pos, const ZNVector3& target, const ZNVector3& up)
{
	// DirectX LookAtLH (Left-Handed coordinate system)
	ZNVector3 zAxis = (target - pos).Normalize();  // Forward direction
	ZNVector3 xAxis = ZNVector3::Cross(up, zAxis).Normalize();  // Right direction
	ZNVector3 yAxis = ZNVector3::Cross(zAxis, xAxis).Normalize();  // Up direction

	float tX = -ZNVector3::Dot(xAxis, pos);
	float tY = -ZNVector3::Dot(yAxis, pos);
	float tZ = -ZNVector3::Dot(zAxis, pos);

	// Row-major view matrix for mul(v, M) with row_major in HLSL
	// Rotation is transposed, translation goes in row 3
	ZNMatrix4 mat(
		xAxis.x, yAxis.x, zAxis.x, 0.0f,
		xAxis.y, yAxis.y, zAxis.y, 0.0f,
		xAxis.z, yAxis.z, zAxis.z, 0.0f,
		tX, tY, tZ, 1.0f);

	SetView(mat);
}

void ZNFramework::ZNCamera::SetView(const ZNMatrix4& m)
{
	this->viewMatrix = m;
}

void ZNFramework::ZNCamera::SetProjection(const ZNMatrix4& m)
{
	this->projectionMatrix = m;
}

void ZNFramework::ZNCamera::SetPerspective(float fov, float aspect, float nearZ, float farZ)
{
	// DirectX perspective projection (Left-Handed)
	// For mul(v, M) with row_major in HLSL
	float yScale = 1.0f / tan(fov / 2.0f);
	float xScale = yScale / aspect;
	float fRange = farZ / (farZ - nearZ);

	ZNMatrix4 mat(
		xScale, 0.0f, 0.0f, 0.0f,
		0.0f, yScale, 0.0f, 0.0f,
		0.0f, 0.0f, fRange, 1.0f,
		0.0f, 0.0f, -fRange * nearZ, 0.0f);

	SetProjection(mat);
}

void ZNFramework::ZNCamera::SetOrthographic(float width, float height, float nearZ, float farZ)
{
	// Orthographic projection for mul(v, M) with row_major in HLSL
	// DirectX left-handed, depth range [0, 1]
	ZNMatrix4 mat(
		2.0f / width, 0.0f, 0.0f, 0.0f,
		0.0f, 2.0f / height, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f / (farZ - nearZ), 0.0f,
		0.0f, 0.0f, -nearZ / (farZ - nearZ), 1.0f);

	SetProjection(mat);
}

// Camera transform
void ZNCamera::SetPosition(const ZNVector3& pos)
{
	position = pos;
	if (autoUpdateView)
		UpdateViewMatrix();
}

void ZNCamera::SetRotation(float inPitch, float inYaw, float roll)
{
	// Convert degrees to radians
	const float DEG_TO_RAD = 3.14159265f / 180.0f;
	pitch = inPitch * DEG_TO_RAD;
	yaw = inYaw * DEG_TO_RAD;
	UpdateVectors();
	if (autoUpdateView)
		UpdateViewMatrix();
}

void ZNCamera::SetTarget(const ZNVector3& target)
{
	ZNVector3 direction = (target - position).Normalize();

	// Calculate pitch and yaw from direction
	pitch = asin(-direction.y);
	yaw = atan2(direction.x, direction.z);

	UpdateVectors();
	if (autoUpdateView)
		UpdateViewMatrix();
}

// Camera movement
void ZNCamera::MoveForward(float distance)
{
	position = position + forward * distance;
	if (autoUpdateView)
		UpdateViewMatrix();
}

void ZNCamera::MoveRight(float distance)
{
	position = position + right * distance;
	if (autoUpdateView)
		UpdateViewMatrix();
}

void ZNCamera::MoveUp(float distance)
{
	position = position + up * distance;
	if (autoUpdateView)
		UpdateViewMatrix();
}

// Camera rotation
void ZNCamera::RotatePitch(float angle)
{
	pitch += angle;

	// Clamp pitch to avoid gimbal lock
	const float maxPitch = 1.55f; // ~89 degrees
	if (pitch > maxPitch)
		pitch = maxPitch;
	if (pitch < -maxPitch)
		pitch = -maxPitch;

	UpdateVectors();
	if (autoUpdateView)
		UpdateViewMatrix();
}

void ZNCamera::RotateYaw(float angle)
{
	yaw += angle;
	UpdateVectors();
	if (autoUpdateView)
		UpdateViewMatrix();
}

// Input handling
void ZNCamera::ProcessKeyboard(const KeyboardEvent& event, float deltaTime)
{
	if (event.state != KEY_STATE::DOWN && event.state != KEY_STATE::PRESS)
		return;

	float velocity = moveSpeed * deltaTime;

	switch (event.type)
	{
	case KEY_TYPE::KEY_W:
		MoveForward(velocity);
		break;
	case KEY_TYPE::KEY_S:
		MoveForward(-velocity);
		break;
	case KEY_TYPE::KEY_A:
		MoveRight(-velocity);
		break;
	case KEY_TYPE::KEY_D:
		MoveRight(velocity);
		break;
	case KEY_TYPE::KEY_Q:
		MoveUp(-velocity);
		break;
	case KEY_TYPE::KEY_E:
		MoveUp(velocity);
		break;
	}
}

void ZNCamera::ProcessMouse(const MouseEvent& event, float sensitivity)
{
	if (event.state == MOUSE_STATE::DOWN)
	{
		// Start dragging - initialize last position
		isDragging = true;
		lastMouseX = event.x;
		lastMouseY = event.y;
	}
	else if (event.state == MOUSE_STATE::UP)
	{
		// Stop dragging
		isDragging = false;
	}
	else if (event.state == MOUSE_STATE::MOVE && isDragging)
	{
		// Only rotate while dragging
		float xOffset = static_cast<float>(event.x - lastMouseX) * sensitivity;
		float yOffset = static_cast<float>(event.y - lastMouseY) * sensitivity;

		RotateYaw(xOffset);
		RotatePitch(-yOffset); // Inverted

		lastMouseX = event.x;
		lastMouseY = event.y;
	}
}

// Matrix helpers
ZNMatrix4 ZNCamera::ViewProjectionMatrix() const
{
	return viewMatrix * projectionMatrix;
}

ZNMatrix4 ZNCamera::GetMVP(const ZNMatrix4& modelMatrix) const
{
	return modelMatrix * viewMatrix * projectionMatrix;
}

// Update view matrix from position and orientation
void ZNCamera::UpdateViewMatrix()
{
	ZNVector3 target = position + forward;
	SetView(position, target, up);
}

// Update direction vectors from pitch/yaw
void ZNCamera::UpdateVectors()
{
	// Calculate new forward vector
	ZNVector3 newForward;
	newForward.x = sin(yaw) * cos(pitch);
	newForward.y = sin(pitch);
	newForward.z = cos(yaw) * cos(pitch);
	forward = newForward.Normalize();

	// Calculate right and up vectors (DirectX left-handed)
	ZNVector3 worldUp(0.0f, 1.0f, 0.0f);
	right = ZNVector3::Cross(worldUp, forward).Normalize();
	up = ZNVector3::Cross(forward, right).Normalize();
}
