#pragma once
// perspective
// view matrix
// projection matrix -- perspective projection, parallel projection
#include "../ZNInclude.h"
#include "Math/ZNMatrix4.h"

namespace ZNFramework
{
	struct KeyboardEvent;
	struct MouseEvent;

	// Tunable camera-control parameters, gathered in one place (were scattered: moveSpeed on the
	// camera, sensitivity passed into ProcessMouse each call). Exposed via ControlConfig() so the
	// Debug panel can drive them with sliders at runtime.
	struct CameraControlConfig
	{
		float moveSpeed   = 5.0f;    // world units / second
		float sensitivity = 0.002f;  // radians per pixel of mouse drag
	};

	class ZNCamera
	{
	public:
		ZNCamera();

		// Legacy: view matrix (kept for backward compatibility)
		void SetView(const ZNVector3& pos, const ZNVector3& target, const ZNVector3& up);
		void SetView(const ZNMatrix4& m);
		void SetProjection(const ZNMatrix4& m);

		// projection matrix
		void SetPerspective(float fov, float aspect, float nearZ, float farZ);
		void SetOrthographic(float width, float height, float nearZ, float farZ);

		// Camera transform
		void SetPosition(const ZNVector3& pos);
		void SetRotation(float pitch, float yaw, float roll = 0.0f); // Angles in radians
		void SetTarget(const ZNVector3& target); // Look at target

		ZNVector3 GetPosition() const { return position; }
		ZNVector3 GetForward() const { return forward; }
		ZNVector3 GetRight() const { return right; }
		ZNVector3 GetUp() const { return up; }
		float GetPitch() const { return pitch; }
		float GetYaw() const { return yaw; }

		// Camera movement (First-Person style)
		void MoveForward(float distance);
		void MoveRight(float distance);
		void MoveUp(float distance);

		// Camera rotation (First-Person style)
		void RotatePitch(float angle); // Up/Down (X-axis rotation)
		void RotateYaw(float angle);   // Left/Right (Y-axis rotation)

		// Input handling.
		// ProcessMovement: frame-based (polled) movement. localIntent components are the desired
		// motion along the camera's own axes — x=right, y=up, z=forward, each typically -1/0/+1
		// from the held key set. The vector is normalized (so diagonals aren't faster) then scaled
		// by moveSpeed*deltaTime. Call once per frame from the app loop, not per key event.
		void ProcessMovement(const ZNVector3& localIntent, float deltaTime);
		void ProcessMouse(const MouseEvent& event);

		// Matrix getters
		ZNMatrix4 ViewMatrix() const { return this->viewMatrix; }
		ZNMatrix4 ProjectionMatrix() const { return this->projectionMatrix; }
		ZNMatrix4 ViewProjectionMatrix() const;
		ZNMatrix4 GetMVP(const ZNMatrix4& modelMatrix) const;

		// Update camera (call this every frame if using transform-based camera)
		void UpdateViewMatrix();

		// Camera settings
		void SetMoveSpeed(float speed) { control.moveSpeed = speed; }
		float GetMoveSpeed() const { return control.moveSpeed; }
		CameraControlConfig&       ControlConfig()       { return control; }
		const CameraControlConfig& ControlConfig() const { return control; }

	private:
		void UpdateVectors(); // Update forward, right, up vectors from pitch/yaw

	private:
		ZNMatrix4 viewMatrix;
		ZNMatrix4 projectionMatrix;

		// Transform-based camera
		ZNVector3 position;
		ZNVector3 forward;
		ZNVector3 right;
		ZNVector3 up;

		float pitch; // X-axis rotation (radians)
		float yaw;   // Y-axis rotation (radians)

		// Camera settings
		CameraControlConfig control;
		bool autoUpdateView; // Auto update view matrix when position/rotation changes

		// Mouse input state
		bool isDragging; // Track if mouse button is being held
		int lastMouseX;
		int lastMouseY;
	};
}