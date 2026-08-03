#pragma once
#include <imgui.h>
#include <imgui_internal.h>   // FindWindowByName, for GetWindowHeightByName below

namespace ZNFramework
{
	// Which viewport corner a panel is pinned to.
	enum class UICorner { TopLeft, TopRight, BottomLeft, BottomRight };

	// Pin the NEXT ImGui window to a viewport corner with an outer margin.
	//
	// Position is set every frame (ImGuiCond_Always) using the matching pivot, so the window keeps
	// hugging its corner across window resize / maximize instead of drifting (a one-shot
	// SetNextWindowPos is computed once for the old viewport size and then goes stale). A corner-
	// anchored window is therefore not free-dragged — it always snaps back to its corner.
	//
	// `size`: usual ImGui convention (0 on an axis = auto-fit that axis). Applied as FirstUseEver so
	// the window stays user-resizable and the size persists in imgui.ini; only the anchor is forced.
	// Pass margin < 0 to reuse the caller's default.
	inline void AnchorNextWindow(UICorner corner, const ImVec2& size, float margin)
	{
		const ImGuiViewport* vp = ImGui::GetMainViewport();
		const ImVec2 origin = vp->WorkPos;   // top-left of the usable area (excludes any menu bar)
		const ImVec2 area   = vp->WorkSize;  // usable size

		ImVec2 pos, pivot;
		switch (corner)
		{
		case UICorner::TopLeft:     pos = ImVec2(origin.x + margin,          origin.y + margin);          pivot = ImVec2(0.f, 0.f); break;
		case UICorner::TopRight:    pos = ImVec2(origin.x + area.x - margin, origin.y + margin);          pivot = ImVec2(1.f, 0.f); break;
		case UICorner::BottomLeft:  pos = ImVec2(origin.x + margin,          origin.y + area.y - margin); pivot = ImVec2(0.f, 1.f); break;
		case UICorner::BottomRight: pos = ImVec2(origin.x + area.x - margin, origin.y + area.y - margin); pivot = ImVec2(1.f, 1.f); break;
		default:                    pos = ImVec2(origin.x + margin,          origin.y + margin);          pivot = ImVec2(0.f, 0.f); break;
		}

		ImGui::SetNextWindowPos(pos, ImGuiCond_Always, pivot);
		if (size.x != 0.f || size.y != 0.f)
			ImGui::SetNextWindowSize(size, ImGuiCond_FirstUseEver);
	}

	// Last known height (px) of a window by title, so a second panel can stack directly beneath it
	// regardless of collapse state / content size. 0 if that window hasn't been drawn yet this run.
	inline float GetWindowHeightByName(const char* name)
	{
		if (ImGuiWindow* w = ImGui::FindWindowByName(name))
			return w->Size.y;
		return 0.0f;
	}
}
