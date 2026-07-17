#pragma once
#include <cstdint>

namespace ZNFramework
{
	// stable handle to a scene-owned gameobject. generation is bumped on slot reuse, so a handle
	// to a freed/reused slot goes stale -> Resolve() returns null instead of dangling.
	// use this over the raw pointer for refs that outlive a frame.
	struct ZNObjectHandle
	{
		uint32_t index      = 0;
		uint32_t generation = 0;  // 0 = null

		bool IsNull() const { return generation == 0; }
		bool operator==(const ZNObjectHandle& o) const { return index == o.index && generation == o.generation; }
		bool operator!=(const ZNObjectHandle& o) const { return !(*this == o); }
	};
}
