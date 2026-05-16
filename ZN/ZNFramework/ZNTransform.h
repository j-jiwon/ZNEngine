#pragma once
#include "Math/ZNVector3.h"
#include "Math/ZNMatrix4.h"
#include "Math/ZNLinearTransform3.h"
#include "Math/ZNMath.h"

namespace ZNFramework
{
	struct Transform
	{
		ZNVector3 position = ZNVector3(0.f, 0.f, 0.f);
		ZNVector3 rotation = ZNVector3(0.f, 0.f, 0.f); // Euler angles (X, Y, Z) in degrees
		ZNVector3 scale = ZNVector3(1.f, 1.f, 1.f);

		// Calculate world matrix from position, rotation, and scale
		ZNMatrix4 GetWorldMatrix() const
		{
			// Convert degrees to radians
			float rotX = ConvertDegreesToRadians(rotation.x);
			float rotY = ConvertDegreesToRadians(rotation.y);
			float rotZ = ConvertDegreesToRadians(rotation.z);

			// Create scale matrix
			ZNMatrix4 scaleMatrix;
			scaleMatrix.m[0][0] = scale.x;
			scaleMatrix.m[1][1] = scale.y;
			scaleMatrix.m[2][2] = scale.z;

			// Create rotation matrices
			ZNLinearTransform3 rotTransform;
			rotTransform.RotateX(rotX); // Pitch
			rotTransform.RotateY(rotY); // Yaw
			rotTransform.RotateZ(rotZ); // Roll

			// Convert 3x3 rotation to 4x4 matrix
			ZNMatrix4 rotMatrix;
			rotMatrix.m[0][0] = rotTransform.matrix3.m[0][0];
			rotMatrix.m[0][1] = rotTransform.matrix3.m[0][1];
			rotMatrix.m[0][2] = rotTransform.matrix3.m[0][2];
			rotMatrix.m[1][0] = rotTransform.matrix3.m[1][0];
			rotMatrix.m[1][1] = rotTransform.matrix3.m[1][1];
			rotMatrix.m[1][2] = rotTransform.matrix3.m[1][2];
			rotMatrix.m[2][0] = rotTransform.matrix3.m[2][0];
			rotMatrix.m[2][1] = rotTransform.matrix3.m[2][1];
			rotMatrix.m[2][2] = rotTransform.matrix3.m[2][2];

			// Create translation matrix
			ZNMatrix4 translationMatrix;
			translationMatrix.m[0][3] = position.x;
			translationMatrix.m[1][3] = position.y;
			translationMatrix.m[2][3] = position.z;

			// HLSL cbuffer uses column-major packing (matrix is transposed)
			// So we send T * R * S, which becomes S * R * T after transpose
			return translationMatrix * rotMatrix * scaleMatrix;
		}
	};
}
