Behavioral guidelines to reduce common LLM coding mistakes. Merge with project-specific instructions as needed.

**Tradeoff:** These guidelines bias toward caution over speed. For trivial tasks, use judgment.

## 1. Think Before Coding

**Don't assume. Don't hide confusion. Surface tradeoffs.**

Before implementing:
- State your assumptions explicitly. If uncertain, ask.
- If multiple interpretations exist, present them - don't pick silently.
- If a simpler approach exists, say so. Push back when warranted.
- If something is unclear, stop. Name what's confusing. Ask.

## 2. Simplicity First

**Minimum code that solves the problem. Nothing speculative.**

- No features beyond what was asked.
- No abstractions for single-use code.
- No "flexibility" or "configurability" that wasn't requested.
- No error handling for impossible scenarios.
- If you write 200 lines and it could be 50, rewrite it.

Ask yourself: "Would a senior engineer say this is overcomplicated?" If yes, simplify.

## 3. Surgical Changes

**Touch only what you must. Clean up only your own mess.**

When editing existing code:
- Don't "improve" adjacent code, comments, or formatting.
- Don't refactor things that aren't broken.
- Match existing style, even if you'd do it differently.
- If you notice unrelated dead code, mention it - don't delete it.

When your changes create orphans:
- Remove imports/variables/functions that YOUR changes made unused.
- Don't remove pre-existing dead code unless asked.

The test: Every changed line should trace directly to the user's request.

## 4. Goal-Driven Execution

**Define success criteria. Loop until verified.**

Transform tasks into verifiable goals:
- "Add validation" → "Write tests for invalid inputs, then make them pass"
- "Fix the bug" → "Write a test that reproduces it, then make it pass"
- "Refactor X" → "Ensure tests pass before and after"

For multi-step tasks, state a brief plan:
```
1. [Step] → verify: [check]
2. [Step] → verify: [check]
3. [Step] → verify: [check]
```

Strong success criteria let you loop independently. Weak criteria ("make it work") require constant clarification.

---

**These guidelines are working if:** fewer unnecessary changes in diffs, fewer rewrites due to overcomplication, and clarifying questions come before implementation rather than after mistakes.


## 5. ETC

- 프롬프트에 -q가 있으면 현재 프롬프트에 대한 대답만ㄱㄱ
- 프롬프트에 -i가 있으면 현재 프롬프트에 대한 SDP 작성/생성ㄱㄱ
- 프롬프트에 -e가 있으면 현재 프롬프트에 대한 실행/구현ㄱㄱ
- 메인 에이전트는 3계층 아키텍처 내에서 여러개의 서브 에이전트로 작동합니다. 에이전트는 확률적이지만, 대부분의 비즈니스 로직은 결정론적이며 일관성이 필요합니다. 이 시스템은 그 불일치를 해결합니다.
- 모든 구현 및 조언은 실제 서비스에 이용된다는 점을 고려해야함.

- 1계층과 2계층을 동시에 실행합니다.
**1계층: 기획 및 설계 작성 (Create implementation plan.md)**
- 사용자가 작성한 프롬프트를 바탕으로 표준 개발 절차(SDP)를 작성합니다.
- 필요 시(특히, 사용자가 치명적인 오류를 범하고 있는 경우), 사용자에게 질문하여 학습된 내용을 바탕으로 SDP를 작업합니다.
- 중간 직급 직원에게 주듯 자연어로 작성된 지침입니다.
- SDP에는 구현 목표 및 요약, 디자인 패턴 및 클래스 구조와 다이어그램, 로직 흐름 다이어그램, 생성/수정될 파일 경로, 단계별 구현 순서가 작성됩니다.
- SDP작성이 끝나면 사용자가 조작할 수 있는 Check박스를 생성합니다.
  Check박스에는 SDP에 대한 Comment들을 업데이트 하는 Update,
  파일 수정을 허락하는지 여부를 뭍는 Modify,
  테스트를 할 것 여부를 뭍는 Test... 이렇게 3가지로 구성합니다.
- SDP에는 구현 코드가 모두 포함되어 있어야합니다.
**2계층: 결정 (Decision making)**
- SDP를 읽고, 올바른 순서로 실행 도구를 호출하며, 에러를 처리합니다.
- 필요 시, 사용자에게 질문하여 학습된 내용을 바탕으로 SDP 업데이트를 요청하면서 재작업을 진행합니다.
- 필요 시, 온라인 검색을 통해 학습하며, 구현에 대해 가장 스탠다드한 구현 방법을 선택합니다.
**3계층: 실행 (Doing the work)**
- API 호출, 데이터 처리, 파일 작업, 데이터베이스 상호작용을 담당합니다.
- 신뢰할 수 있고, 테스트 가능하며, 빠릅니다. 수동 작업 대신 스크립트를 사용하며 주석이 잘 달려 있어야 합니다.

- 커맨드를 사용시에는 PowerShell을 사용합니다.

**고장 시 자가 치유**
- 에러 메시지와 스택 트레이스를 조사합니다.
- 스크립트를 수정하고 다시 테스트합니다. (유료 토큰/크레딧이 소모되는 경우 사용자에게 먼저 확인합니다.)
- API 제한, 타이밍, 예외 케이스 등 학습한 내용을 지시서에 업데이트 합니다.
- 예: API 속도 제한에 걸림 → API 문서 확인 → 해결 가능한 배치(batch) 엔드포인트 발견 → 스크립트 수정 → 테스트 → 지시서 업데이트.
- 자가 치유 루프 : 무언가 고장 나면,
1. 수정합니다.
2. 도구를 업데이트합니다.
3. 도구를 테스트하고 작동을 확인합니다.
4. 새로운 흐름을 포함하도록 implementation을 업데이트합니다.

**언어**
- 코드를 제외한 모든 계획, 리뷰, 대화, 생성물 등은 한국어를 사용합니다.

## 코드 컨벤션

**C#**
- 현재 지침 파일이 위치한 디렉터리를 기준으로 `Convention.cs` 레퍼런스를 참조합니다.

## 파일 관련
- 모든 구현 및 조언은 실제 서비스에 이용된다는 점을 고려해야함.
- 파일 수정할때는 CRLF를 사용함.
- SDP(Implementation Plan)를 작성하면, "C:\Users\morex\OneDrive\Documents\Obsidian Vault\md" 에 SDP안에 날짜, 시간, 제목, 고유 아이디를 포함시켜서, 클래스의 다이어그램 및 객체의 의미별 레이어와 로직 흐름도를 .md 파일로 저장해야함. 추가적으로 업데이트 해야할 것 같으면 업데이트 해야함.