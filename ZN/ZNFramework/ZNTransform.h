#pragma once
#include "Math/ZNVector3.h"
#include "Math/ZNMatrix4.h"
#include "Math/ZNLinearTransform3.h"

namespace ZNFramework
{
	struct Transform
	{
		ZNVector3 position = ZNVector3(0.f, 0.f, 0.f);
		ZNVector3 rotation = ZNVector3(0.f, 0.f, 0.f); // Euler angles (X, Y, Z) in radians
		ZNVector3 scale = ZNVector3(1.f, 1.f, 1.f);

		// Calculate world matrix from position, rotation, and scale
		ZNMatrix4 GetWorldMatrix() const
		{
			// Create scale matrix (identity by default constructor)
			ZNMatrix4 scaleMatrix;
			scaleMatrix.m[0][0] = scale.x;
			scaleMatrix.m[1][1] = scale.y;
			scaleMatrix.m[2][2] = scale.z;

			// Create rotation matrices (ZYX order - roll, yaw, pitch)
			ZNLinearTransform3 rotTransform;
			rotTransform.RotateX(rotation.x); // Pitch
			rotTransform.RotateY(rotation.y); // Yaw
			rotTransform.RotateZ(rotation.z); // Roll

			// Convert 3x3 rotation to 4x4 matrix
			ZNMatrix4 rotMatrix; // Identity by default
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
			ZNMatrix4 translationMatrix; // Identity by default
			translationMatrix.m[0][3] = position.x;
			translationMatrix.m[1][3] = position.y;
			translationMatrix.m[2][3] = position.z;

			// Combine: Scale -> Rotation -> Translation (SRT)
			return scaleMatrix * rotMatrix * translationMatrix;
		}
	};
}
