# ZNEngine

A real-time 3D rendering engine written from scratch in C++ and Direct3D 12 — no Unreal, no Unity.
Its main demo binds sensor-style vehicle tracking data to a scene graph through an interface the
renderer knows nothing about, so the data source can be swapped between synthetic traffic and
recorded log replay without the scene noticing.

![Instanced draws: 7,267 to 60 draw calls](Docs/images/vehicle_instancing_toggle.gif)

**[Demo video (2 min)](https://youtu.be/_2CfJv7DidY)** · **[Case study](Docs/CASE_STUDY.md)** · **[Windows build](https://github.com/j-jiwon/ZNEngine/releases/latest)**

---

## What it does

**Automotive surround-view demo** — the scene the engine was built to drive.

- `IDataSource → FrameData → SceneBinding → scene graph`; synthetic traffic and JSON log replay
  are interchangeable behind one interface
- Sensor Hz resampled to display fps, with dropout, jitter and latency injectable at runtime
- Front / rear / left / right perspective views plus a top-down orthographic view, rendered
  offscreen and composited
- Record the live stream to JSON, then scrub, seek and loop it back

**Renderer**

- Deferred shading into 6 G-Buffer targets, HDR SceneColor, bloom, tonemapping
- PBR with split-sum image-based lighting from a captured cubemap
- Analytic disco caustics — light spots solved by inverting the mirror ball's facet grid
- Render graph with automatic resource-state transitions
- Slot/handle scene ownership with generation counters, so freed handles invalidate themselves
- Per-pass GPU timestamps, draw-call counter and a live G-Buffer preview, built in

---

## Instanced draws

The demo submits the same geometry seven times per frame — one shadow pass, five surround views,
one main view. Batching objects that share a mesh makes the draw-call count independent of how
many objects are in the scene:

| | 81 objects | 1,747 objects |
|---|---|---|
| Draw calls, per-object | 407 | 7,267 |
| Draw calls, instanced | **60** | **60** |
| Frame time | 16.22 → **8.09 ms** | 201.54 → **31.09 ms** |
| GPU frame time | 6.41 → **4.16 ms** | 76.26 → **12.79 ms** |

Identical triangle counts on both sides. RTX 3060, 1920×1080, Release x64, VSync off.
The measurement setup, the per-pass breakdown and two results that contradicted the obvious guess
are in the **[case study](Docs/CASE_STUDY.md)**.

---

## Scenes

| Scene | What it shows |Scene|
|---|---|---|
| **Vehicle** | The automotive demo — data sources, resampling, surround views, instancing |![VehicleScene](Docs/Images/vehicle_x1_instancing_on.png)|
| **MirrorBall** | PBR, image-based lighting, analytic disco caustics |![MirrorBallScene](Docs/images/mirrorball.png)|
| **CCTV** | Offscreen camera rendered into a material on a TV mesh in the same scene |![CCTVScene](Docs/images/cctv.png)|
| **Test** | Roughness × metallic sphere grid for shading validation |![TestScene](Docs/images/testscene.png)|

---

## Building

```
Visual Studio 2022 · C++17 · Windows 10/11 x64
```

Open `ZNEngine.sln`, build **Release | x64**, run `TestApp`.
Assets are resolved relative to the executable, falling back to the repository layout when running
from the build directory, so both `F5` and a packaged folder work.

To produce a distributable folder:

```powershell
.\Tools\package.ps1
```

Prebuilt binaries are on the [releases page](https://github.com/j-jiwon/ZNEngine/releases/latest).
They need the [Visual C++ Redistributable (x64)](https://aka.ms/vs/17/release/vc_redist.x64.exe) —
the release build links the CRT dynamically.

---

## Layout

```
ZN/ZNFramework/          engine
  Application/           app loop and initialisation
  Window/                Win32 window and events
  Graphics/              renderer; Platform/Direct3D12 holds the implementation
    Platform/Direct3D12/Passes/    render-graph passes
  Scene/                 ZNScene, ZNGameObject, ZNObjectHandle
  Math/                  vector / matrix / transform (no DirectXMath)
  UI/                    ImGui layer
TestApp/Framework/       sample scenes
  Vehicle/               automotive subsystem: sources, interpolation, binding, log format
Resources/               shaders, models, textures
Docs/                    case study, third-party licences
```

The engine is split into a platform-agnostic interface layer (`ZNXxx`) and a Direct3D 12
implementation behind factory functions, so `TestApp` never sees a D3D12 type.

---

## Licence

Engine source is [BSD 3-Clause](LICENSE). Third-party libraries and assets are credited in
[Third-Party Licenses](Docs/THIRD_PARTY_LICENSES.md).
