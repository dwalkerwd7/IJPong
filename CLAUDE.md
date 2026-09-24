# It's Just Pong

Unreal Engine 5.8 C++ game (single runtime module `IJPong`). Working concept, "The Lie": it starts as a faithful 1972 Pong that slowly unravels into something bigger. The chapter ideas in the original plan (`~/.claude/plans/i-d-like-to-plan-reactive-newell.md`) are **tentative, not a spec**.

## How we work
- **Step by step, as a learning exercise.** Claude writes the code in small chunks and walks through it. The user reviews before we move on. One step per turn, never several systems at once.
- **The user makes the design and architecture decisions.** At each step, present the real choices with a recommendation, then wait. Don't decide silently.
- **Build present-first.** Build what the player sees first, add general reusable features as we go, and design levels/chapters later from features that already exist. Don't justify decisions with speculative future chapters.
- The user is comfortable in UE C++: explain architecture and game-specific techniques, not UCLASS/UPROPERTY basics.
- **Every step ends with a build and a commit.** Claude commits at the end of each step without asking. Commit messages end with the Co-Authored-By line.

## Testing
**Integration tests only. No unit tests.** Tests go in `Source/IJPong/Tests/`. They create a real world with arena, ball and paddles, run the simulation, and check gameplay outcomes. Use the `FIJPTestWorld` fixture (`Tests/IJPTestWorld.h`). It runs the real game mode and BeginPlay path and ticks at a fixed 1/60 s.

Run headless (the editor must not hold the DLL; see Build):
```
"C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "<repo>\IJPong.uproject" -ExecCmds="Automation RunTests IJPong.; Quit" -unattended -nullrhi -nopause -nosplash -NoSound -log=IJPTests.log
```
Then grep `Saved/Logs/IJPTests.log` for `Test Completed`. To test without closing the user's editor, copy `IJPong.uproject`, `Source`, `Config` and `Content` to a **short** path such as `C:\IJPT`, then build and test there, and delete it afterwards. The scratchpad path is too long for UBT.

## Build
```
"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" IJPongEditor Win64 Development -Project="<repo>\IJPong.uproject" -WaitMutex -NoHotReloadFromIDE
```
The "banned MSVC 14.40–14.43" lines in the output are just UBT listing toolchains it skips (it uses 14.51). They aren't errors.

**If the editor is open, the build compiles but can't link**: the editor locks `Binaries/Win64/UnrealEditor-IJPong.dll`. Reopening the editor afterwards does *not* rebuild it. The editor quietly loads the old DLL, and new classes are missing (`Failed to find object 'Class /Script/IJPong...'`). Live Coding is unsafe for steps that add UCLASSes. The user has OK'd Claude doing this routine:
1. Close the editor gracefully: `(Get-Process UnrealEditor).CloseMainWindow()`, then `Wait-Process -Timeout 90`. It's the same as clicking X, so unsaved work raises the editor's save dialog. If it's still running afterwards, tell the user a dialog is waiting. **Never `Stop-Process` it.**
2. Build in place, then check the DLL's timestamp and size to confirm it really relinked.
3. Relaunch: `Start-Process "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe" -ArgumentList '"<repo>\IJPong.uproject"'`.

**Shader edits (`Shaders/*.ush`)**: the editor caches shader source files. After editing one, restart the editor (routine above) and then recompile `M_CRT` (MCP `MaterialTools.recompile`, which raises on HLSL errors). Recompiling without a restart silently uses the old file.

## Editor access (Unreal MCP)
The engine's experimental `ModelContextProtocol` plugin is enabled, along with the Editor, AutomationTest, LiveCoding and UMG toolsets. The server runs inside the editor at `http://127.0.0.1:8000/mcp` (`unreal-mcp` in `.mcp.json`, auto-start on). Its tools exist only while the editor is open, and they disconnect during a close/rebuild/relaunch. Ask before doing anything destructive in the user's open editor.
- `call_tool` takes the **short** tool name (e.g. `create`), not the fully-qualified name `describe_toolset` lists.
- `DataAssetTools.create` can make `UDataAsset` subclasses such as InputAction and InputMappingContext. `ObjectTools.set_properties` edits them. Instanced sub-objects (e.g. IMC modifiers) are passed as class paths.

## Decisions so far
- Vs AI only, no networking.
- C++ owns rules and simulation. Data Assets and BP own tuning and content.
- The ball is a custom kinematic, sweep-based mover (not Chaos).
- Git + LFS: `.uasset`/`.umap`/`.png`/`.wav`/`.fbx` are tracked by LFS.
- The `PongBall` trace channel defaults to **Ignore**: objects must opt in to block the ball.
- Scoring = **goal boxes** that block the ball's sweep (not a goal-line position check).
- Paddle bounce: **60°** max edge angle, **no spin** (pure 1972).
- Game modes subclass **`AIJPGameModeBase`** (finds the arena, player gets the left paddle, AI the right). **`AIJPTestGameMode`** is the project default for now: endless play of any level, with debug keys R (reset score), F (serve now), T (hand the player's paddle to an AI), -/= (opponent skill) and `.` (debug overlay; not F1, which the engine binds to wireframe view in development builds; `AIJPTestHUD`, on by default; its lines come from `AIJPTestGameMode::GetDebugLines`, so add new tool state there). Real modes (match rules, chapters) come later as separate subclasses.
- The AI is an **`AAIController`** so behaviour trees can drive abilities later.
- Paddles are tuned by **`UIJPPaddleProfile`** Data Assets (size, max speed, ramp time, flicker time), one per side on each arena (`LeftPaddleProfile` / `RightPaddleProfile`, default `/Game/Paddles/DA_Paddle_Default` via config). New paddle variants are new assets, not code.
- **No difficulty settings in the game.** Difficulty is a designer-authored spectrum: each arena's `OpponentSkill` (0 hopeless .. 1 near-perfect) is read through `FIJPSkillRange` pairs (value at skill 0 / at skill 1) in the `UIJPAIProfile`. Skill 0.5 = the original tuning. In the test mode, `-` / `=` nudge it live.
- Sound: the ball broadcasts events (`OnPaddleHit`, `OnBounce`, `OnGoal`). The **arena** maps them to beeps from a `UIJPToneSet` Data Asset, played by a C++ square-wave synth (`UIJPToneSynthComponent`). The synth suits this 1972 level, but **the user wants custom sound assets eventually**. Add those at the arena's event → sound mapping (e.g. an optional `USoundBase` per event), not in the ball.
- Screen look: **CRT post-process** (scanlines, gentle curvature + vignette, phosphor glow) via `UIJPCRTComponent`, which works on **any camera**, not just the arena's (the user wants it on menu cameras too). Material `M_CRT` (a Custom node that `#include`s `Shaders/IJPCRT.ush`), tuned in `MI_CRT_1972`, set as the default in config.
- Playfield is **4:3, pillarboxed**. Scores are **chunky seven-segment** digits. Walls are visible by default (`bShowWalls` toggle; the user hasn't decided this yet).
