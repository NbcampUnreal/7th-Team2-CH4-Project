# Tiny Warfare (CH4_Project)

SPARTA 내일배움캠프 언리얼 7기 2팀, 챌린지반 4차 팀 프로젝트.
2026-04-01 ~ 2026-04-24 (약 3주 반), 팀원 6명.

---

## 프로젝트 소개

**Tiny Warfare**: 언리얼 엔진 5.6 기반 리슨 서버 멀티플레이 RTS입니다. 상대 본진(Nexus)을 파괴하면 승리하는 단일 조건 아래에서, 플레이어는 **생산 → 정찰 → 점령 → 전투**의 RTS 루프를 따라갑니다. 스타크래프트의 단순한 규칙과 깊은 선택, Dinolord의 현대적인 유닛 조합, Company of Heroes의 점령·테크·자원 사이클을 레퍼런스로 삼아 3주 반 안에 "플레이가 성립하는" 수준까지 끌어올리는 것이 목표였습니다.

### 주요 기능

- 리슨 서버 기반 2인 멀티플레이와 로비 → 본게임 심리스 트래블
- Mass Entity(ECS) 위에 얹은 커스텀 복제 계층으로 대량 유닛 네트워크 동기화
- 영웅 3종(DragonKnight · Markman · Astrologian), 데이터 테이블 기반 스킬 반경/쿨타임
- 그리드 스냅 건설, 수중·지형 밖 건설 차단, 고스트 미리보기, 연구소 테크
- 전장의 안개 + 미니맵, 팀 기반 가시성 매트릭스, 미니맵 클릭 카메라 점프
- 서버 권위 드래그 선택 및 명령 우선순위 (내 영웅 → 내 일반 유닛 → 내 건물 → 적 유닛)
- 일반 점령지 / 특수 점령지 / 넥서스로 구성된 맵 오브젝트
- CommonUI 기반 HUD, Provider/Coordinator 패턴의 단방향 데이터 파이프라인

### 기술 스택

C++17 (UE 5.6), Mass Entity (ECS), StateTree / GameplayStateTree, CommonUI, Enhanced Input, Niagara, NavMesh + NavCorridor.

---

## 개발 환경

- **Engine**: Unreal Engine 5.6
- **Language**: C++17
- **IDE**: JetBrains Rider 또는 Visual Studio 2022 (MSVC v143)
- **Platform**: Windows 10 / 11 (x64)
- **Build Tool**: Unreal Build Tool
- **Git Flow**: `feature/*` → `develop` → `main`

### 빌드 / 실행

1. `.uproject` 우클릭 → `Switch Unreal Engine version...` 로 5.6 선택
2. `.uproject` 우클릭 → `Generate Visual Studio project files`
3. `CH4_ProjectEditor` (Development Editor, Win64) 타깃 빌드
4. 에디터 실행 후 `Net Mode: Play As Listen Server`, `Number of Players ≥ 2` 로 PIE

### 모듈 / 플러그인

- **UBT 의존 모듈** (`CH4_Project.Build.cs`): `MassEntity`, `MassCommon`, `MassMovement`, `MassActors`, `MassSpawner`, `MassAIBehavior`, `MassNavigation`, `MassNavMeshNavigation`, `MassSignals`, `MassReplication`, `MassEQS`, `MassLOD`, `StateTreeModule`, `GameplayStateTreeModule`, `EnhancedInput`, `CommonUI`, `Niagara`, `NavCorridor`, `Landscape`, `AIModule`, `NavigationSystem`, `GameplayTags`, `NetCore`
- **활성 플러그인** (`.uproject`): `MassGameplay`, `MassCrowd`, `MassAI`, `StateTree`, `GameplayStateTree`, `CommonUI`, `ModelingToolsEditorMode` (Editor-only)
- **In-tree 플러그인**: `Plugins/UEGitPlugin` (Rider/Git 연동)

---

## 프로젝트 구조

```less
7th-Team2-CH4-Project
 ┣ Config/                          // DefaultEngine.ini (맵 경로, 심리스 트래블 설정)
 ┣ Content/                         // 언리얼 에셋 (.uasset, 커밋 대상)
 ┃ ┗ CH4_Project/Maps/Main/         // L_Title, L_Lobby, 본게임 레벨
 ┣ Plugins/UEGitPlugin/             // Editor-only, Rider/Git 연동
 ┣ Source/
 ┃ ┣ CH4_Project.Target.cs          // 게임 타깃
 ┃ ┣ CH4_ProjectEditor.Target.cs    // 에디터 타깃
 ┃ ┣ CH4_Project/                   // 런타임 주 모듈
 ┃ ┃ ┗ Private|Public/
 ┃ ┃   ┣ Core/                      // GameMode, GameState, PlayerController, PlayerState
 ┃ ┃   ┣ Lobby/                     // 로비 전용 GameMode/PC/PS, TWLobby_Layout
 ┃ ┃   ┣ Title/                     // 타이틀 (호스트/참가)
 ┃ ┃   ┣ Building/                  // Nexus / Population / Resource / TroopSpawn / Upgrade / Ghost
 ┃ ┃   ┣ HeroUnit/                  // 영웅 액터, AreaEffect, Buff, Projectile
 ┃ ┃   ┣ Mass/
 ┃ ┃   ┃ ┣ Fragments/               // Unit / Command / Attack / Status / ClientVelocity / Steering / Avoidance / TransformOffset
 ┃ ┃   ┃ ┣ Processor/               // Command / TargetSearch / Chasing / Attack / AttackAnimation / NavMeshPathFollow / Orientation / Despawn / EASync / TransformSmoothOffset / Avoidance / Steering
 ┃ ┃   ┃ ┣ StateTree/               // StateTree 태스크·평가자
 ┃ ┃   ┃ ┣ Traits/                  // Mass Trait 조합
 ┃ ┃   ┃ ┗ Replication/             // 멀티플레이어 복제 설정
 ┃ ┃   ┣ Subsystems/                // UnitSubsystem, BuildingManagerSubsystem, GridSubSystem, MassBubbleTickGuardSubsystem, SoundManagerSubsystem
 ┃ ┃   ┣ CapturePoint/              // 점령지, 게이지, 팀 컬러
 ┃ ┃   ┣ FOW/                       // FogManager, VisionComponent
 ┃ ┃   ┣ Selection/                 // 드래그/클릭 선택, SelectionVisualActor
 ┃ ┃   ┣ Minimap/                   // 미니맵 위젯·아이콘
 ┃ ┃   ┣ Component/                 // TeamComponent, TeamColorComponent, BuildComponent, PlayerUIControllerComponent, PlayerSelectionVisualComponent
 ┃ ┃   ┣ Animation/                 // Mass 연동 애님 인스턴스/노티파이
 ┃ ┃   ┣ UI/
 ┃ ┃   ┃ ┣ Core/                    // PlayerUIBridge, UIHUDCoordinator, UISelectionProvider, UIResourceProvider, SelectionDataProvider, ResourceDataProvider
 ┃ ┃   ┃ ┣ Widgets/                 // HUDRoot, SelectionPanel, CommandPanel, MinimapPanel, MenuPanel 등
 ┃ ┃   ┃ ┗ Data/                    // UIDataTypes, InputStateTypes
 ┃ ┃   ┣ Cheat/                     // CheatManager 확장 (Development 전용)
 ┃ ┃   ┣ GamePlay/                  // 보조 액터/유틸
 ┃ ┃   ┣ Data/                      // TW*BuildingDataAsset, TWHeroTableRowBase, TWUnitTableRowBase, TWUpgradeTableRowBase
 ┃ ┃   ┣ Interface/                 // TWInputKeyHandler, TWPlayerSlotInterface
 ┃ ┃   ┗ Log/                       // TWLogCategory (12종 카테고리)
 ┃ ┗ GitStatusBranch/               // Editor-only, Rider/Git 편의 모듈 (런타임 로직 금지)
 ┣ CH4_Project.uproject
 ┣ CLAUDE.md
 ┗ README.md
```

**네이밍**: 게임플레이 C++ 클래스는 `TW` 접두사에 UE 표준 접두사(`A/U/F/E`)를 붙입니다 (예: `ATWGameMode`, `UTWBuildComponent`, `FTWUnitFragment`, `ETWCommandId`). 블루프린트 에셋은 `BP_` · `WBP_` · `DT_` · `DA_` · `L_` 접두사를 사용합니다.

---

## 핵심 구현 포인트

### 1. 서버 권위 초기화와 심리스 트래블

`ATWGameMode`가 Login → 슬롯/팀 할당 → Nexus 지정 → 영웅 스폰까지 한 번에 관장합니다. 로비에서 결정한 팀·닉네임·영웅 선택은 `ATWPlayerState`에 남긴 뒤 `HandleSeamlessTravelPlayer`로 본게임 레벨에 그대로 넘어갑니다. 레벨과 함께 소멸하는 액터에 상태를 두지 않고 PlayerState · 전역 서브시스템 · GameInstance 중 하나에만 두는 원칙을 지킵니다.

### 2. Mass Entity + 커스텀 복제

Mass의 기본 복제는 RTS 수준 응답성에 부족했기 때문에, Fragment 상태 변화를 감지해 **필요한 항목만 서버에서 클라이언트로 내려보내는** 커스텀 복제 계층을 제작했습니다. 애니메이션은 RPC 없이 Fragment 데이터 변화를 보고 재생되게 했고, 클라이언트 시각적 이동은 `TWClientVelocityFragment`와 `TWTransformSmoothOffsetProcessor`의 오프셋 보간으로 끊김 없이 흐르도록 만들었습니다. 프로세서 구성은 `TWCommandProcessor`, `TWTargetSearchProcessor`, `TWChasingProcessor`, `TWAttackProcessor`, `TWAttackAnimationProcessor`, `TWNavMeshPathFollowProcessor`, `TWOrientationProcessor`, `TWDespawnProcessor`, `TWEASyncProcessor`, `TWAvoidanceProcessors`, `TWSteeringProcessors`로 분할되어 있습니다.

### 3. 단일 그리드 좌표계로 시스템 통합

건설 판정 · 시야 렌더링 · 길찾기 · 미니맵이 각자 다른 좌표계를 쓰면 버그가 폭발적으로 늘기 때문에, `UTWGridSubSystem`에 **월드 좌표 ↔ 그리드 좌표 변환 레이어**를 고정해두고 이후 모든 기능이 이 변환을 경유하도록 설계했습니다. 미니맵 클릭이 월드 카메라 이동으로, 건설 고스트 칸이 실제 건물 충돌 박스로 오차 없이 매핑됩니다.

### 4. 영웅 스킬 해석 파이프라인 (데이터 주도)

`ATWPlayerController`의 `ResolveHeroSkillRowName` · `ResolveHeroSkillTargetRadius` · `ResolveHeroSkillDamageRadius` · `ResolveHeroSkillAcquireRadius` · `ResolveHeroSkillImpactRadius` · `ResolveHeroSkillImpactLifeTime`이 `DT_HeroTableRowBase`에서 값을 읽어옵니다. `DamageRadius` 필드가 비어 있으면 `TargetDecalRadius` → `SkillRange` → 기본값(`HeroSkillDefaultTargetDecalRadius`) 순으로 내려가는 fallback 체인을 가지고 있어, 데이터 누락으로 스킬이 동작하지 않는 상황을 방지합니다. 수치 조정은 코드 수정 없이 데이터 테이블에서 가능합니다.

### 5. 전장의 안개와 팀 기반 가시성

유닛·건물·점령지에 붙는 `UTWVisionComponent`와 `UTWTeamComponent` 조합으로 팀 가시성 매트릭스를 구성합니다. 한 번 지나간 지역은 반투명으로 남고 현재 시야만 밝은 영역으로 그려지도록 렌더 타겟을 기록 방식으로 운용합니다. `ATWFogManager`가 미니맵의 `ShouldShow`와도 공유되어, 적 유닛이 과거 시야 구역에서 아예 식별되지 않도록 처리됩니다.

### 6. 서버 권위 드래그 선택 / 명령

클릭 거리 기반으로 단일 클릭과 드래그를 한 프로세서에서 가르고, 드래그 범위에 잡힌 대상은 **내 영웅 → 내 일반 유닛 → 내 건물 → 적 유닛** 순서로 필터링됩니다. 적은 정보 조회만 허용되며 실제 명령은 아군에만 들어갑니다. 클라이언트가 보낸 명령이라도 서버가 `UTWTeamComponent`의 TeamID를 한 번 더 확인해 소유권 없는 대상은 원천 차단합니다.

### 7. 단방향 UI 데이터 파이프라인

UI는 데이터를 직접 제어하지 않습니다. 상태 변화는 `ATWPlayerController` → `UTWPlayerUIControllerComponent` → `UTWPlayerUIBridge` → `UTWUIHUDCoordinator` + `UTWUISelectionProvider` + `UTWUIResourceProvider` → Widget 순으로 단방향 전파되고, Widget의 if 분기는 Provider 레이어로 끌어올려 표준화했습니다. 선택 유닛의 HP, 건설/연구 진행도, 커맨드 쿨타임이 모두 이 한 경로로 흐릅니다.

### 8. 심리스 트래블 Mass 크래시 방지

초기에 로비 → 본게임 전환 중 `AMassClientBubbleInfoBase`가 tear-down 시점에 틱하여 크래시가 발생했습니다. `UTWMassBubbleTickGuardSubsystem`을 도입해 `OnWorldBeginTearDown`에서 버블 틱을 비활성화하는 방식으로 해결했고, 해당 서브시스템은 재발 방지 목적이라 현재도 유지됩니다.

---

## 과제 추가 구현 사항

팀 요구사항 외에 독자적으로 추가한 항목입니다.

- **리슨 서버 기반 멀티플레이 RTS**: 단일 플레이 요구를 2인 호스트/클라이언트 구조로 확장
- **Mass Entity ECS + 커스텀 복제**: 대량 유닛 네트워크 안정성을 위한 자체 복제 계층
- **그리드 좌표계 통일**: 건설·시야·길찾기·미니맵을 한 좌표계로 묶어 버그 표면 축소
- **렌더 타겟 기반 전장의 안개**: 탐색 흔적 + 실시간 시야 이중 레이어
- **미니맵 클릭 → 월드 카메라 점프**: 좌표 쌍방 변환 일반화
- **영웅 3종 + 데이터 주도 스킬 해석**: 데이터 테이블 fallback 체인
- **로비 → 본게임 심리스 트래블**: 영웅/팀/닉네임 PlayerState 유지
- **팀 컬러 컴포넌트 분리**: 메시 수정 없이 색상만 교체 가능한 구조
- **Provider/Coordinator 단방향 UI 파이프라인**: Widget의 if 분기 최소화

---

## 주의사항

- **엔진 버전 고정**: UE 5.6 필수. 다른 버전으로 열면 Mass/StateTree 플러그인 호환이 깨집니다.
- **체크아웃 직후 프로젝트 파일 재생성**: `.sln` · `Binaries/` · `Intermediate/` · `DerivedDataCache/` · `Saved/`는 gitignore 대상입니다.
- **`GitStatusBranch` 모듈은 Editor-only**: 런타임 게임플레이 코드를 여기에 넣으면 쿠킹 빌드에서 누락됩니다.
- **리슨 서버 검증 필수**: 호스트는 서버와 로컬 클라이언트 경로를 동시에 타므로, 순수 `IsServer`/`IsClient` 분기만으로는 UI·유닛 인식이 어긋날 수 있습니다. 호스트 경로 별도 검증 필요.
- **심리스 트래블 중 보존이 필요한 상태**는 PlayerState · 전역 서브시스템 · GameInstance 중 한 곳에만 둡니다.
- **시작 자원 10000 (`TWPlayerState.cpp`)**: 테스트 편의로 박혀 있는 값입니다. 릴리스 전 밸런스 값으로 교체 필요.
- **치트/배속**: Development 빌드 전용이며 Shipping에서는 비활성화됩니다.
- **`TWMassBubbleTickGuardSubsystem`**: 심리스 트래블 크래시 방지 전용입니다. 제거/수정 금지.

---

## 팀 구성

| 이름 | 담당 |
| --- | --- |
| 조창민 (팀장) | 전투 · AI · 이동 |
| 김남태 (부팀장) | 기획 · 네트워크 |
| 김태우 (PM) | 영웅 유닛 · 레벨 · 미니맵 |
| 이희원 | 깃 관리 · UI |
| 유시환 | 서기 · 로비 |
| 박둘내 | 건설 · 밸런스 · 발표 |

---

## 협업 / 커밋 규약

- 기본 브랜치 `main`(릴리스), 통합 브랜치 `develop`, 작업 브랜치 `feature/<이름>`
- 커밋 메시지는 `[작성자이름] 한국어 요약` 형식을 기본으로 하며, `feat:` / `fix:` 스타일도 혼재합니다
- `Config/` · `Content/`는 바이너리지만 커밋 대상이므로, 병합 충돌은 언리얼 에디터의 Diff/Merge 도구로 해결합니다