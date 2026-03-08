#pragma once

using int8 = __int8;
using int16 = __int16;
using int32 = __int32;
using int64 = __int64;
using uint8 = unsigned __int8;
using uint16 = unsigned __int16;
using uint32 = unsigned __int32;
using uint64 = unsigned __int64;

#include <filesystem>

#include "ZNFramework/Math/ZNPoint.h"
#include "ZNFramework/Math/ZNVector2.h"
#include "ZNFramework/Math/ZNVector3.h"
#include "ZNFramework/Math/ZNVector4.h"

#include "ZNFramework/Math/ZNMatrix2.h"
#include "ZNFramework/Math/ZNMatrix3.h"
#include "ZNFramework/Math/ZNMatrix4.h"

#include "ZNFramework/Math/ZNLinearTransform2.h"
#include "ZNFramework/Math/ZNLinearTransform3.h"
#include "ZNFramework/Math/ZNAffineTransform2.h"
#include "ZNFramework/Math/ZNAffineTransform3.h"

#include "ZNFramework/ZNColor.h"
#include "ZNFramework/ZNCamera.h"
#include "ZNFramework/ZNTimer.h"

#include "ZNFramework/ZNInputDef.h"

#include "ZNFramework/Application/ZNApplication.h"
#include "ZNFramework/Window/ZNWindow.h"
#include "ZNFramework/Graphics/ZNGraphicsDevice.h"
#include "ZNFramework/Graphics/ZNCommandQueue.h"
#include "ZNFramework/Graphics/ZNSwapChain.h"
#include "ZNFramework/Graphics/ZNRootSignature.h"
#include "ZNFramework/Graphics/ZNMesh.h"
#include "ZNFramework/Graphics/ZNShader.h"
#include "ZNFramework/Graphics/ZNConstantBuffer.h"
#include "ZNFramework/Graphics/ZNTableDescriptorHeap.h"
#include "ZNFramework/Graphics/ZNTexture.h"
#include "ZNFramework/Graphics/ZNDepthStencilBuffer.h"
#include "ZNFramework/Graphics/ZNMaterial.h"

#include "ZNFramework/Graphics/ZNGraphicsContext.h"

#include "ZNFramework/Graphics/Platform/Direct3D12/ZNUtils.h"
//#include "ZNFramework/ZNGeometry.h" 


namespace ZNFramework
{
	struct Vertex 
	{
		Vertex() {}

		Vertex(ZNVector3 p, ZNVector4 c, ZNVector2 u)
			: pos(p), color(c), uv(u)
		{
		}
	
		ZNVector3 pos;
		ZNVector4 color;
		ZNVector2 uv;
	};

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

	enum class TextureType : uint8
	{
		Albedo = 0,
		Normal = 1,
		Metallic = 2,
		Roughness = 3,
		AO = 4,
		Count = 5
	};

	struct MaterialParams
	{
		ZNVector4 albedoColor = ZNVector4(1.f, 1.f, 1.f, 1.f);
		float metallic = 0.0f;
		float roughness = 0.5f;
		float ao = 1.0f;
		float padding = 0.0f; // 16-byte alignment
	};
}

inline std::filesystem::path GetExecutablePath()
{
	wchar_t buffer[MAX_PATH];
	GetModuleFileNameW(nullptr, buffer, MAX_PATH);
	std::filesystem::path exePath(buffer);
	return exePath.parent_path();
}

inline std::filesystem::path GetResourcePath()
{
	std::filesystem::path ResourcePath = GetExecutablePath().parent_path().parent_path() / L"Resources";
	return ResourcePath;
}
