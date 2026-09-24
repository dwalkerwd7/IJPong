# IJPong module: code conventions

## Layout
Includes are relative to the module root (`PublicIncludePaths.Add(ModuleDirectory)`), e.g. `#include "Gameplay/IJPArena.h"`.
- `Core/`: shared types, game framework classes (GameMode, GameState, PlayerController)
- `Gameplay/`: arena, ball, paddles, goals, maths
- `Presentation/`: screen look. `UIJPCRTComponent` puts the CRT post-process on every camera of its owner. The HLSL is in the project's `Shaders/IJPCRT.ush` (mapped to `/IJPong` by the tiny `IJPongShaders` module).
- `AI/`: paddle AI controller and its `UIJPAIProfile` Data Asset. The AI drives paddles through the same `AddMoveInput` as the player.
- `Tests/`: integration tests only

Class prefix is `IJP` (`AIJPArena`, `UIJPGoalComponent`, `FIJPPongMath`). The log category is `LogIJPong`. File headers are `// It's Just Pong`.

## Plane space
All gameplay is simulated in 2D **arena plane space** (see `Core/IJPTypes.h`):
- plane X = arena local X (goals at −X / +X)
- plane Y = arena local Z (walls at −Z / +Z)
- arena local Y is depth; the arena camera sits at +Y looking toward −Y.

Only `AIJPArena` converts between plane and world space (`PlaneToWorld`, `WorldToPlane`, `PlaneDirToWorld`, `WorldDirToPlane`). Plane space ignores actor scale. Change `HalfExtents`, not actor scale.

## Collision
- The ball sweeps on `ECC_PongBall` (`ECC_GameTraceChannel1`, a trace channel, default Ignore).
- Anything the ball should hit calls `IJP::ConfigureAsBallBlocker` (query-only, blocks only PongBall). Purely visual pieces call `IJP::ConfigureAsVisualOnly`.
- Keep collision and visuals as separate components. Ball blockers are deep along local Y (`BlockerDepth`) so small depth offsets can't miss.

## Rendering pieces
Pong visuals are `/Engine/BasicShapes/Cube` instances (100 units, centred) scaled into boxes, using the arena's `PongMaterial` (default `/Engine/EngineMaterials/EmissiveMeshMaterial`, unlit). Layout is rebuilt in `OnConstruction`, so properties update live in the editor.

## Maths
Stateless Pong maths lives in `FIJPPongMath` (paddle bounce, reflect, angle clamp, wall-folding intercept prediction). Reuse it rather than re-deriving bounce logic.
