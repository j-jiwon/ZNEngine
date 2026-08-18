# Third-Party Licenses

This document lists the source and license information for third-party libraries and resource data included with ZNEngine.

## Damaged Helmet

- File: `Resources/Models/DamagedHelmet.glb`
- Source: [KhronosGroup glTF Sample Assets](https://github.com/KhronosGroup/glTF-Sample-Assets/tree/main/Models/DamagedHelmet)
- Original model: Copyright 2016 theblueturtle_, licensed under the [Creative Commons Attribution-NonCommercial 4.0 International License](https://creativecommons.org/licenses/by-nc/4.0/)
- glTF rebuild and conversion: Copyright 2018 ctxwing, licensed under the [Creative Commons Attribution 4.0 International License](https://creativecommons.org/licenses/by/4.0/)

The model was rebuilt and converted to conform to the final glTF 2.0 specification. See the [upstream legal information](https://github.com/KhronosGroup/glTF-Sample-Assets/blob/main/Models/DamagedHelmet/README.md#legal) for details.

## Low Poly: Isometric Room

- File: `Resources/Models/room.glb`
- Original model: Low Poly: Isometric Room
- Creator: Mehmet Nizam Saltan (`@nizamsaltan`)
- Source: [Sketchfab](https://skfb.ly/oy78F)
- Original model license: [Creative Commons Attribution 4.0 International](https://creativecommons.org/licenses/by/4.0/)
- Original textures: Included with the original model and licensed under CC BY 4.0 by Mehmet Nizam Saltan.
- Modifications: Converted to glTF binary format and partially retextured for use in ZNEngine.
- Additional texture photographs: Copyright 2026 Jung Jiwon.

## Kenney Car Kit

- Files: `Resources/Models/sedan-sports.glb`, `Resources/Models/Textures/colormap.png`
- Creator: Kenney
- Source: [Kenney Car Kit](https://kenney.nl/assets/car-kit)
- License: [Creative Commons CC0 1.0 Universal](https://creativecommons.org/publicdomain/zero/1.0/)
- Usage: Vehicle models and textures used by `VehicleScene`.

## HDRI Night

- File: `Resources/Textures/night_free_Bg.jpg`
- Creator: HDRI Hub
- Source: [HDRI Night (free)](https://www.hdri-hub.com/hdrishop/freesamples/freehdri/item/74-hdr-night)
- License: [Creative Commons Attribution 3.0 Unported](https://creativecommons.org/licenses/by/3.0/)
- Usage: Night environment and skybox texture used by `MirrorBallScene`.

## Stanford Bunny

- File: `Resources/Models/stanford-bunny.fbx`
- Source: [Stanford 3D Scanning Repository](https://graphics.stanford.edu/data/3Dscanrep/#bunny)
- Creator: Stanford University Computer Graphics Laboratory
- Reconstruction: Greg Turk and Marc Levoy
- Usage terms: The repository permits research use and free redistribution with source acknowledgement. Commercial use requires permission from Stanford under the repository's stated terms.
- Modifications: Converted to FBX format for use in ZNEngine.

## Qwantani Noon HDRI

- File: `Resources/Textures/qwantani_noon.jpg`
- Creators: Greg Zaal (photography), Jarod Guest (processing)
- Source: [Poly Haven](https://polyhaven.com/a/qwantani_noon)
- License: [Creative Commons CC0 1.0 Universal](https://creativecommons.org/publicdomain/zero/1.0/)
- Usage: Daytime environment texture used by `TestGameScene`.

## Libraries

### Assimp 6.0.5

- Files: `Library/Include/assimp`, `Library/Lib`, `Library/Bin/x64/assimp-vc143-mt.dll`
- Project: [Open Asset Import Library](https://github.com/assimp/assimp)
- License: BSD 3-Clause

Copyright (c) 2006-2026, assimp team

All rights reserved.

Redistribution and use of this software in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the assimp team, nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission of the assimp team.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

### Dear ImGui 1.92.9 WIP

- Files: `ZN/ZNFramework/ThirdParty/imgui`
- Project: [Dear ImGui](https://github.com/ocornut/imgui)
- License: MIT
- Copyright: Copyright (c) 2014-2026 Omar Cornut and Dear ImGui contributors

### JSON for Modern C++ 3.12.0

- File: `ZN/ZNFramework/ThirdParty/nlohmann/json.hpp`
- Project: [nlohmann/json](https://github.com/nlohmann/json)
- License: MIT
- Copyright: Copyright (c) 2013-2025 Niels Lohmann

The following MIT License terms apply to Dear ImGui and JSON for Modern C++:

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
