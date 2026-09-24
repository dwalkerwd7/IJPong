// It's Just Pong

#pragma once

#include "CoreMinimal.h"
#include "IJPTypes.generated.h"

class UPrimitiveComponent;

DECLARE_LOG_CATEGORY_EXTERN(LogIJPong, Log, All);

/** Trace channel the ball sweeps against. Declared as "PongBall" in DefaultEngine.ini (default response: Ignore). */
#define ECC_PongBall ECC_GameTraceChannel1

/**
 * Arena "plane space": the 2D space the game is simulated in.
 *   Plane X == arena local X (horizontal, goals at -X / +X)
 *   Plane Y == arena local Z (vertical, walls at -Z / +Z)
 * Arena local Y is depth; the arena camera sits on +Y looking toward -Y.
 */

UENUM(BlueprintType)
enum class EIJPSide : uint8
{
	Left,
	Right
};

UENUM(BlueprintType)
enum class EIJPMatchPhase : uint8
{
	None,
	/** Ball blinking at the centre, waiting to be served (also the pause after each point). */
	Serve,
	Rally,
	MatchOver
};

namespace IJP
{
	inline EIJPSide Opposite(EIJPSide Side) { return Side == EIJPSide::Left ? EIJPSide::Right : EIJPSide::Left; }

	/** +1 for the right side, -1 for the left side (plane X direction pointing toward that side's goal). */
	inline float SideSign(EIJPSide Side) { return Side == EIJPSide::Left ? -1.f : 1.f; }

	/** Make a primitive block only the ball's sweep, and nothing else. */
	IJPONG_API void ConfigureAsBallBlocker(UPrimitiveComponent* Component);

	/** Make a primitive purely visual. */
	IJPONG_API void ConfigureAsVisualOnly(UPrimitiveComponent* Component);
}
