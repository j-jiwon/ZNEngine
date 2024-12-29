#pragma once

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

#include "ZNFramework/Application/ZNApplication.h"
#include "ZNFramework/Window/ZNWindow.h"
#include "ZNFramework/Graphics/ZNGraphicsDevice.h"
#include "ZNFramework/Graphics/ZNCommandQueue.h"
#include "ZNFramework/Graphics/ZNSwapChain.h"
#include "ZNFramework/Graphics/ZNRootSignature.h"
#include "ZNFramework/Graphics/ZNMesh.h"
#include "ZNFramework/Graphics/ZNShader.h"

#include "ZNFramework/Graphics/ZNGraphicsContext.h"

#include "ZNFramework/Graphics/Platform/Direct3D12/ZNUtils.h"
//#include "ZNFramework/ZNGeometry.h" 

namespace ZNFramework
{
	struct Vertex 
	{
		Vertex() {}

		Vertex(ZNVector3 p, ZNVector4 c)
			: pos(p), color(c)
		{
		}
	
		ZNVector3 pos;
		ZNVector4 color;
	};
}

inline std::filesystem::path GetExecutablePath()
{
	wchar_t buffer[MAX_PATH];
	GetModuleFileNameW(nullptr, buffer, MAX_PATH);
	std::filesystem::path exePath(buffer);
	return exePath.parent_path();
}
