#pragma once
#include "ZNFramework/UI/ImGuiAnchor.h"  // ZNFramework::AnchorNextWindow + UICorner (corner pinning)

// Central layout constants for the debug/editing UI (SceneDebugUI). Tweak the panel width,
// positions and default heights HERE instead of hunting SetNextWindowPos/Size calls scattered
// through the panels. All panels share one width so the UI reads as a single column/bar system.
namespace UILayout
{
    constexpr float Margin     = 10.0f;    // gap from viewport edges and between panels
    constexpr float PanelWidth = 240.0f;   // uniform width for every panel

    // Pin the next window to a viewport corner (re-anchored every frame, so it tracks window
    // resize / maximize). Use for panels that should hug an edge instead of being placed once:
    //   UILayout::Anchor(ZNFramework::UICorner::TopRight, ImVec2(UILayout::PanelWidth, 0.f));
    inline void Anchor(ZNFramework::UICorner corner, const ImVec2& size, float margin = Margin)
    {
        ZNFramework::AnchorNextWindow(corner, size, margin);
    }

    // --- Left column (top -> bottom): Stats, then the merged Outliner/Inspector ---
    constexpr float StatsX = Margin, StatsY = Margin;                   // top-left, auto height
    constexpr float OutlinerX = Margin, OutlinerY = 220.0f;            // below Stats
    constexpr float OutlinerHeight  = 560.0f;                          // merged panel default (resizable)
    constexpr float HierarchyHeight = 300.0f;                          // scrollable tree area inside it

    // --- Top bar (left -> right, to the right of the left column): Scenes, Debug ---
    constexpr float ScenesX = Margin + PanelWidth + Margin,          ScenesY = Margin;
    constexpr float DebugX  = Margin + (PanelWidth + Margin) * 2.0f,  DebugY  = Margin;

    // --- Right edge: GBuffer preview (engine-drawn; shares PanelWidth for uniformity) ---
    constexpr float GBufferWidth = PanelWidth;
}
