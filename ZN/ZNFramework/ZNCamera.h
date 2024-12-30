#pragma once
// perspective
// view matrix
// projection matrix -- perspective projection, parallel projection 
#include "../ZNInclude.h"
#include "Math/ZNMatrix4.h"

namespace ZNFramework
{
	class ZNCamera
	{
	public:
		ZNCamera();

		// view matrix
		void SetView(const ZNVector3& pos, const ZNVector3& target, const ZNVector3& up);
		void SetView(const ZNMatrix4& m);
		void SetProjection(const ZNMatrix4& m);

		// projection matrix
		void SetPerspective(float fov, float aspect, float nearZ, float farZ);
		void SetOrthographic(float width, float height, float nearZ, float farZ);
		
		ZNMatrix4 ViewMatrix() const { return this->viewMatrix; };
		ZNMatrix4 ProjectionMatrix() const { return this->projectionMatrix; };

	private:
		ZNMatrix4 viewMatrix;
		ZNMatrix4 projectionMatrix;

	};
}