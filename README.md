# ZNEngine

D3D12 기반 커스텀 렌더링 엔진. 서드파티 프레임워크 없이 3층 추상화(엔진 / 플랫폼 / 앱), 자체 수학 라이브러리, Win32·D3D12 초기화까지 직접 구현했다. 현재는 이 렌더러 위에서 **실시간 센서 데이터를 3D로 시각화하는 automotive 데모**로 확장 중이다.

<!-- 대표 영상 또는 gif URL 한 줄 (인라인 재생됨) -->

---

디퍼드 + HDR + IBL + PostProcess 파이프라인, RenderGraph 기반 패스 관리, ImGui 디버그/에디팅 UI. `DirectXMath`에 의존하지 않는다.

| 렌더링 | 데이터 | 도구 |
|---|---|---|
| 디퍼드 셰이딩(MRT 5), PBR(Cook-Torrance) | `IDataSource → FrameData → 씬 바인딩` | Outliner / Inspector 실시간 편집 |
| split-sum IBL, 환경 큐브맵, 스카이박스 | 멀티카메라 서라운드뷰(오프스크린 RT) | GBuffer 채널 프리뷰, 와이어프레임 |
| HDR, Bloom, 톤매핑, 디스코 코스틱 | 로그 재생 / 합성 소스 | GPU/CPU 프로파일링 패널 |

**설계 원칙 — 3층 분리.** 콘텐츠 코드는 D3D12를 전혀 알지 못한다. 이 경계가 Vulkan 백엔드 포팅의 토대가 된다.

---

## 개발 로그

각 마일스톤을 스크린샷·gif·영상, 관련 PR과 함께 정리해 두었다.

| 단계 | 내용 |
|---|---|
| 아키텍처 | 3층 추상화, 자체 수학 라이브러리, 렌더 골격 |
| PBR / IBL | Cook-Torrance, split-sum IBL, 디스코 코스틱 |
| 오프스크린 렌더 | 오프스크린 카메라, CCTV→TV 합성 |
| HDR 파이프라인 | HDR, Bloom, 톤매핑 |
| 구조 리팩터링 | 계층 모델, 소유권/핸들, 렌더 순회 통합 |
| Automotive | 실시간 3D 시각화, 멀티카메라 서라운드뷰 |

**→ [전체 개발 로그 (CHRONICLE.md)](./CHRONICLE.md)**

---

## 빌드 / 실행

- **환경:** Windows, D3D12
- **의존성:** Assimp, Dear ImGui(소스 포함), GoogleTest
- **솔루션:** `ZNEngine.sln` — `ZN`(엔진) + `TestApp`(샘플 씬)

```
<!-- 실제 빌드 커맨드/스텝으로 교체 -->
```
