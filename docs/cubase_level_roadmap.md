# 큐베이스 수준 피아노롤 개발 로드맵

## 현재 상태 (MVP)

### ✅ 구현된 기능
- 기본 피아노롤 뷰 (그리드 + 건반)
- 노트 입력 (더블클릭)
- 노트 선택/이동 (싱글/멀티)
- Lasso 선택
- 노트 삭제
- 코드 스탬핑 (TheoryToolbar 연동)
- 스케일 하이라이트
- Undo/Redo 시스템
- 벨로시티 레인 (별도 컴포넌트)
- 페달 레인 (별도 컴포넌트)
- 마우스 휠 Pan (중간 버튼)
- 플레이헤드 표시
- 건반 미리듣기

### ❌ 부족한 부분
- **툴 시스템**: Draw/Select만 있고 Line/Erase/Split 등 없음
- **Tool Modifiers**: 모디파이어 키로 툴 임시 전환 불가
- **UI 레이아웃**: Info Line, Status Line, Inspector 없음
- **노트 편집**: Length 스케일링, Legato, Overlap 조정 불가
- **정밀 편집**: 픽셀 단위 드래그만 가능, 수치 입력 불가
- **컨트롤러 편집**: CC 레인이 있지만 Line Tool 없음
- **퀀타이즈**: UI만 있고 실제 적용 로직 미구현
- **표현**: Note Expression, Dynamics 시각화 부족
- **키보드 단축키**: 거의 없음 (Delete만)
- **줌/스크롤**: 마우스 휠 줌 없음
- **스냅/그리드**: 그리드 표시만 있고 스냅 On/Off 토글 없음

---

## 로드맵: 큐베이스 Key Editor 수준 달성

### Phase 1: 핵심 UI 인프라 (Foundation) - 2-3주
**목표**: 큐베이스와 동일한 "정보 표시 + 편집 인터페이스" 구축

#### 1.1 Info Line 구현
- [ ] `InfoLine` 컴포넌트 생성
- [ ] 선택 노트의 Start/End/Length/Pitch/Velocity 표시
- [ ] 값 직접 편집 가능한 텍스트 필드
- [ ] 다중 선택 시 "Multiple" 표시

#### 1.2 Status Line 구현
- [ ] `StatusLine` 컴포넌트 생성
- [ ] 마우스 위치 (Bar:Beat:Tick, Pitch) 실시간 표시
- [ ] 현재 코드 감지 및 표시 (선택 노트 기반)
- [ ] 스냅/그리드 상태 표시

#### 1.3 Inspector Panel (Left Zone) 구축
- [ ] `InspectorPanel` 컴포넌트 생성
- [ ] Quantize 섹션 (기존 QuantizePanel 통합)
  - [ ] Quantize 실행 버튼
  - [ ] Strength 슬라이더 (0-100%)
  - [ ] Swing 슬라이더
  - [ ] Iterative/Magnetic 모드
- [ ] Note Length 섹션
  - [ ] Scale Length 슬라이더 (-200% ~ +200%)
  - [ ] Scale Legato 슬라이더
  - [ ] Freeze MIDI Lengths 버튼
  - [ ] Overlap 틱 조정 (+/- 버튼)
- [ ] Transpose 섹션
  - [ ] 세미톤 +/- 버튼
  - [ ] 옥타브 +/- 버튼
- [ ] Scale Assistant 섹션 (기존 TheoryToolbar 통합)
  - [ ] Scale Root 선택
  - [ ] Scale Type 선택
  - [ ] ON/OFF 토글
- [ ] Chord Editing 섹션
  - [ ] Chord Stamp 모드 선택
  - [ ] Inversion 옵션

#### 1.4 Toolbar 재구성
- [ ] `PianoRollToolbar` 컴포넌트 생성
- [ ] 툴 버튼 배치 (Select/Draw/Erase/Split/Glue/Mute/Line)
- [ ] Snap ON/OFF 토글 버튼
- [ ] Grid Size 선택 (ComboBox)
- [ ] Zoom 슬라이더 (Horizontal/Vertical)

#### 1.5 Layout Manager
- [ ] MainComponent에서 Info/Status/Toolbar/Inspector 배치
- [ ] Resizable 경계선 (Inspector <-> PianoRoll)
- [ ] "Set up Window Layout" 메뉴로 패널 표시/숨김

---

### Phase 2: Tool System & Modifiers (Workflow Core) - 2-3주
**목표**: 큐베이스 워크플로의 핵심인 "툴 + 모디파이어" 시스템 구축

#### 2.1 Tool Enum & State
- [ ] `EditorTool` enum 정의 (Select, Draw, Erase, Split, Glue, Mute, Line, Trim)
- [ ] `PianoRollView`에 `currentTool` 상태 추가
- [ ] Tool 변경 시 마우스 커서 변경

#### 2.2 Draw Tool 고도화
- [ ] 드래그로 노트 길이 조정 (현재 더블클릭만 가능)
- [ ] Snap 기반 자동 정렬
- [ ] 연속 드로잉 (Paint 모드)
- [ ] 벨로시티 프리셋 적용 (툴바에서 설정)

#### 2.3 Erase Tool
- [ ] 클릭으로 노트 삭제
- [ ] 드래그로 연속 삭제 (Paint Erase)

#### 2.4 Split Tool
- [ ] 노트 클릭 지점에서 분할
- [ ] 스냅 기반 분할 위치 조정
- [ ] 다중 선택 노트 일괄 분할

#### 2.5 Line Tool (Controller Lane용)
- [ ] Line 모드: 직선 그리기
- [ ] Parabola 모드: 곡선 (Ctrl로 반전)
- [ ] Sine/Triangle/Square 모드
- [ ] Snap/Length Quantize로 데이터 밀도 조절
- [ ] Thin Out Data 기능 (과밀 CC 데이터 정리)

#### 2.6 Tool Modifiers 시스템
- [ ] `ToolModifierManager` 클래스 생성
- [ ] Modifier Key 조합 매핑 (Ctrl/Shift/Alt + Tool)
- [ ] Select Tool 대체 액션
  - [ ] Edit Velocity (Shift+Drag)
  - [ ] Set Cursor Position (Ctrl+Click)
  - [ ] Split Event (Alt+Click)
  - [ ] Slip Event Content
- [ ] Preferences에서 Modifier 커스터마이징
  - [ ] Categories: Select/Draw/Erase/Line...
  - [ ] Actions: 각 툴별 대체 동작 목록
  - [ ] Assign: 키 조합 지정

---

### Phase 3: Note Editing (Precision) - 2주
**목표**: 노트 길이/타이밍/피치를 "정밀하게" 편집하는 기능

#### 3.1 Note Resize
- [ ] 노트 끝 드래그로 길이 조정 (현재 ResizeEnd 모드 있지만 미완성)
- [ ] 노트 시작 드래그로 위치+길이 조정 (ResizeStart)
- [ ] Snap 적용/해제 토글
- [ ] Ctrl 누르면 스냅 임시 해제 (Fine Tune)

#### 3.2 Scale Length / Legato
- [ ] `ScaleLengthCommand` 구현
  - [ ] 선택 노트들 길이를 비율로 스케일링 (-200% ~ +200%)
  - [ ] 시작 위치 고정, 끝만 조정
- [ ] `ScaleLegatoCommand` 구현
  - [ ] 다음 노트까지 자동 연결 (갭 제거)
  - [ ] "Extend to Next Selected" 옵션
- [ ] `SetOverlapCommand` 구현
  - [ ] 연속 노트 간 겹침/갭을 틱 단위로 조정
  - [ ] +10틱 버튼, -10틱 버튼

#### 3.3 Freeze MIDI Lengths
- [ ] 현재 길이 설정을 "확정"하는 기능
- [ ] 이후 Quantize 적용 시 길이 유지

#### 3.4 Multi-Note Editing
- [ ] Info Line에서 다중 선택 노트 일괄 수정
  - [ ] Velocity 일괄 설정
  - [ ] Length 일괄 설정 (절대값)
  - [ ] Transpose 일괄 적용

---

### Phase 4: Controller & Expression (Dynamics) - 2주
**목표**: CC와 Note Expression을 "자연스럽게" 편집

#### 4.1 Controller Lane 고도화
- [ ] CC Lane에서 Line Tool 지원
  - [ ] Line/Parabola/Sine 모드 전환 UI
  - [ ] Parabola 모디파이어 (Ctrl=반전, Alt=이동, Shift=지수)
- [ ] CC 이벤트 밀도 관리
  - [ ] Snap ON → Length Quantize로 간격 조절
  - [ ] Snap OFF → 마우스 움직임마다 생성
  - [ ] Thin Out Data 기능 (MIDI 메뉴)

#### 4.2 Velocity Lane 개선
- [ ] 현재 VelocityLane은 별도 컴포넌트
  - [ ] Line Tool로 벨로시티 커브 그리기
  - [ ] 드래그로 연속 조정 (Select Tool + Edit Velocity 모디파이어)
  - [ ] 벨로시티 바 색상으로 강도 시각화 (그라데이션)

#### 4.3 Note Expression (향후 확장)
- [ ] Inspector에 Note Expression 섹션 추가
- [ ] 노트별 표현 파라미터 (Vibrato, Brightness 등)
- [ ] Expression Map 로드 (아티큘레이션)

---

### Phase 5: Quantize Engine (Musical) - 1-2주
**목표**: 실제로 "쓸 수 있는" 퀀타이즈

#### 5.1 Quantize 파라미터 구현
- [ ] Grid (기준 그리드 크기)
- [ ] Swing (0-100%, 홀수 비트 지연)
- [ ] Strength (0-100%, 원본 타이밍 보존 정도)
- [ ] Iterative Quantize (점진적 적용)
- [ ] Magnetic Area (±틱, 범위 내만 적용)

#### 5.2 Quantize Commands
- [ ] `QuantizeCommand` 구현
  - [ ] Start 위치 퀀타이즈
  - [ ] End 위치 퀀타이즈 (옵션)
  - [ ] Strength 적용 (원본과 블렌드)
- [ ] `IterativeQuantizeCommand`
  - [ ] 여러 번 적용 시 점진적으로 정렬
- [ ] Undo/Redo 지원

#### 5.3 Humanize (Anti-Quantize)
- [ ] 랜덤 타이밍 변화 (±틱)
- [ ] 랜덤 벨로시티 변화 (±값)

---

### Phase 6: Keyboard Shortcuts & Workflow - 1주
**목표**: 큐베이스 수준의 "속도감"

#### 6.1 Key Commands Manager
- [ ] `KeyCommandManager` 클래스 생성
- [ ] 액션 ID → 단축키 매핑 테이블
- [ ] Preferences에서 키 재지정

#### 6.2 필수 단축키 (TOP 20)
- [ ] **Tool 전환**
  - [ ] S: Select Tool
  - [ ] D: Draw Tool
  - [ ] E: Erase Tool
  - [ ] X: Split Tool
  - [ ] L: Line Tool
- [ ] **편집**
  - [ ] Ctrl+A: Select All
  - [ ] Ctrl+D: Duplicate
  - [ ] Ctrl+C/V/X: Copy/Paste/Cut
  - [ ] Delete: Delete Selected
  - [ ] Q: Quantize
  - [ ] Ctrl+Q: Quantize Panel
- [ ] **스냅/그리드**
  - [ ] J: Snap ON/OFF
  - [ ] 1-9: Grid Size Preset
- [ ] **줌/네비게이션**
  - [ ] H: Zoom to Fit Horizontal
  - [ ] Shift+F: Zoom to Fit All
  - [ ] G/H: Zoom In/Out Horizontal
  - [ ] Ctrl+마우스휠: Zoom Horizontal
  - [ ] Shift+마우스휠: Zoom Vertical
- [ ] **재생**
  - [ ] Space: Play/Stop
  - [ ] Numpad 0: Return to Zero

#### 6.3 Context Menu
- [ ] 노트 우클릭 메뉴
  - [ ] Quantize
  - [ ] Transpose
  - [ ] Delete
  - [ ] Duplicate
  - [ ] Legato
  - [ ] Split at Cursor
- [ ] 빈 공간 우클릭 메뉴
  - [ ] Paste
  - [ ] Select All
  - [ ] Grid Settings

---

### Phase 7: Zoom & Navigation (UX) - 1주
**목표**: "답답하지 않은" 뷰포트 제어

#### 7.1 Mouse Wheel Zoom
- [ ] Ctrl+Wheel: Horizontal Zoom (마우스 커서 중심)
- [ ] Shift+Wheel: Vertical Zoom
- [ ] Alt+Wheel: Horizontal Scroll

#### 7.2 Zoom Presets
- [ ] Zoom to Fit (전체 노트 보기)
- [ ] Zoom to Selection (선택 노트만)
- [ ] 1:1 Zoom (틱당 1픽셀 같은 기준)

#### 7.3 Minimap (Optional)
- [ ] 상단에 미니맵 표시
- [ ] 전체 클립 개요 + 현재 뷰 박스
- [ ] 드래그로 뷰 이동

---

### Phase 8: Advanced Features (Pro) - 2-3주
**목표**: 큐베이스 Pro 기능 추가

#### 8.1 Chord Track Integration
- [ ] 프로젝트 Chord Track 정보 표시
- [ ] 코드 변화 시점 하이라이트
- [ ] Chord Assistant (자동 코드 감지)

#### 8.2 Global Tracks in Editor
- [ ] Tempo Track 표시
- [ ] Signature Track 표시
- [ ] Marker Track 표시

#### 8.3 MIDI FX Insert
- [ ] Arpeggiator 삽입
- [ ] MIDI Echo
- [ ] MIDI Compressor

#### 8.4 Logical Editor (MIDI Transform)
- [ ] 조건 기반 노트 필터링
- [ ] 배치 변환 (예: "4번째 비트마다 벨로시티 50% 감소")

---

### Phase 9: Performance & Polish - 1주
**목표**: "쾌적한" 사용 경험

#### 9.1 Rendering Optimization
- [ ] Viewport Culling (보이는 노트만 렌더링)
- [ ] Dirty Region 기반 부분 repaint
- [ ] GPU 가속 (JUCE OpenGL)

#### 9.2 Large Project Handling
- [ ] 10,000개 이상 노트 처리
- [ ] 가상화 (Virtual List 기법)

#### 9.3 Visual Feedback
- [ ] 노트 hover 시 하이라이트
- [ ] 드래그 중 고스트 이미지
- [ ] 스냅 가이드라인 표시

---

## 우선순위 제안

### 🔥 Critical (먼저 해야 함)
1. **Phase 1** (UI 인프라): Info Line, Toolbar, Inspector 없으면 정밀 편집 불가
2. **Phase 2** (Tool System): 툴 전환 없으면 워크플로 너무 느림
3. **Phase 6** (Shortcuts): 키보드 없으면 마우스만으로 비효율

### ⚡ High (빠른 시일 내)
4. **Phase 3** (Note Editing): Legato/Scale Length는 실사용 핵심
5. **Phase 5** (Quantize): 현재 UI만 있고 실제 기능 없음
7. **Phase 7** (Zoom): 답답함 해소

### 📌 Medium (점진적)
6. **Phase 4** (Controller): CC 편집은 있지만 Line Tool 필요
8. **Phase 8** (Advanced): Pro 기능은 기본 완성 후

### 🎨 Low (마지막)
9. **Phase 9** (Performance): 최적화는 기능 완성 후

---

## 개발 타임라인 (추정)

| Phase | 기간 | 누적 |
|-------|------|------|
| Phase 1: UI 인프라 | 2-3주 | 3주 |
| Phase 2: Tool System | 2-3주 | 6주 |
| Phase 3: Note Editing | 2주 | 8주 |
| Phase 5: Quantize | 1-2주 | 10주 |
| Phase 6: Shortcuts | 1주 | 11주 |
| Phase 7: Zoom | 1주 | 12주 |
| Phase 4: Controller | 2주 | 14주 |
| Phase 8: Advanced | 2-3주 | 17주 |
| Phase 9: Performance | 1주 | 18주 |

**총 예상 기간: 약 4-5개월 (풀타임 기준)**

---

## 다음 단계

이 로드맵에서 **어떤 Phase부터 시작**하시겠습니까?

추천: **Phase 1 (UI 인프라)** 부터 시작
- Info Line: 선택 노트 정보 표시 + 직접 수정
- Toolbar: 툴 버튼 + Snap 토글 + Grid 선택
- Inspector: Quantize/Length/Transpose 섹션

이것만 해도 "MVP → 실사용 가능" 수준으로 도약합니다.
