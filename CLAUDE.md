# It's Just Pong

Unreal Engine 5.8 C++ game (single runtime module `IJPong`). Working concept, "The Lie": it starts as a faithful 1972 Pong that slowly unravels into something bigger. The chapter ideas in the original plan (`~/.claude/plans/i-d-like-to-plan-reactive-newell.md`) are **tentative, not a spec**.

## How we work
- **Step by step, as a learning exercise.** Claude writes the code in small chunks and walks through it. The user reviews before we move on. One step per turn, never several systems at once.
- **The user makes the design and architecture decisions.** At each step, present the real choices with a recommendation, then wait. Don't decide silently.
- **Build present-first.** Build what the player sees first, add general reusable features as we go, and design levels/chapters later from features that already exist. Don't justify decisions with speculative future chapters.
- The user is comfortable in UE C++: explain architecture and game-specific techniques, not UCLASS/UPROPERTY basics.
- **Every step ends with a build and a commit.** Claude commits at the end of each step without asking. Commit messages end with the Co-Authored-By line.

## Testing
**Integration tests only. No unit tests.** Tests go in `Source/IJPong/Tests/`. They create a real world with arena, ball and paddles, run the simulation, and check gameplay outcomes.

## Build
```
"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" IJPongEditor Win64 Development -Project="<repo>\IJPong.uproject" -WaitMutex -NoHotReloadFromIDE
```
The "banned MSVC 14.40–14.43" lines in the output are just UBT listing toolchains it skips (it uses 14.51). They aren't errors.

## Decisions so far
- Vs AI only, no networking.
- C++ owns rules and simulation. Data Assets and BP own tuning and content.
- The ball is a custom kinematic, sweep-based mover (not Chaos).
- Git + LFS: `.uasset`/`.umap`/`.png`/`.wav`/`.fbx` are tracked by LFS.
- The `PongBall` trace channel defaults to **Ignore**: objects must opt in to block the ball.
- Scoring = **goal boxes** that block the ball's sweep (not a goal-line position check).
- Paddle bounce: **60°** max edge angle, **no spin** (pure 1972).
- Playfield is **4:3, pillarboxed**. Scores are **chunky seven-segment** digits. Walls are visible by default (`bShowWalls` toggle; the user hasn't decided this yet).
