ZNEngine -- Automotive Surround-View Demo
==========================================

QUICK START

1. Extract the entire ZIP before running the demo.
2. Run TestApp.exe. Keep TestApp.exe, assimp-vc143-mt.dll, and Resources\ in
   their original relative locations.
3. The demo starts in MirrorBall Scene. In the Scenes panel, select
   Vehicle Scene to open the automotive surround-view demo.

REQUIREMENTS

- Windows 10 or 11, x64
- A Direct3D 12-capable GPU
- Microsoft Visual C++ Redistributable (x64):
  https://aka.ms/vs/17/release/vc_redist.x64.exe

CAMERA AND UI CONTROLS

- W / S                  move forward / backward
- A / D                  move left / right
- Q / E                  move down / up
- Drag in the 3D view    rotate the camera (any mouse button)
- F2                     show or hide the debug UI
- Speed / Sens.          adjust movement speed and mouse sensitivity in
                         the Debug panel

The small button in the upper-right corner also shows or hides the debug UI.
Close the application window to exit.

WHAT TO TRY

Vehicle Scene

- Live / Load Log
  Switch between synthetic traffic and the included JSON recording. In log
  playback, use Play/Pause, the timeline, Speed, and Loop.

- Record 20s
  While Live is selected, record a new scenario. The result replaces
  Resources\Logs\scenario.json and can be opened with Load Log.

- Instanced draws
  Toggle Instanced draws and compare Draw calls in the Vehicle and Stats
  panels. Select density x1, x5, x20, or x50 while Live is active to stress
  the repeated-mesh workload. Wireframe mode disables instanced rendering.

- Temporal resample
  Compare Off (raw sensor rate), Interpolate, and Extrapolate. Lower Sensor Hz
  or raise Dropout and Jitter to make their behaviour easier to see. The panel
  reports buffered snapshots, added latency, and sample staleness.

- Surround View
  Inspect the front, rear, left, right, and top-down offscreen camera views.

Other scenes

- MirrorBall Scene       PBR, image-based lighting, and analytic disco caustics
- CCTV Scene             an offscreen camera rendered onto a TV material
- Test Scene             a roughness-by-metallic PBR validation grid

TROUBLESHOOTING

- If TestApp.exe reports a missing runtime DLL, install the Visual C++
  Redistributable linked above.
- If assets or shaders fail to load, re-extract the complete ZIP and do not
  move TestApp.exe away from the Resources\ folder.
- Load Log requires Resources\Logs\scenario.json. The official package ships
  with one; Record 20s can create a replacement.

PROJECT LINKS

Source:
https://github.com/j-jiwon/ZNEngine

Case study:
https://github.com/j-jiwon/ZNEngine/blob/main/Docs/CASE_STUDY.md

Demo video:
https://youtu.be/_2CfJv7DidY

LICENCES AND ASSET TERMS

ZNEngine-authored source code and the first-party assets identified in
THIRD_PARTY_LICENSES.md are distributed under the BSD 3-Clause License.
Bundled libraries and third-party assets retain their own terms.

Important commercial-use restrictions:

- Resources\Models\DamagedHelmet.glb is licensed under CC BY-NC 4.0.
- Commercial use of Resources\Models\stanford-bunny.fbx requires Stanford's
  permission.

See LICENSE and THIRD_PARTY_LICENSES.md in this folder before redistribution
or reuse.
