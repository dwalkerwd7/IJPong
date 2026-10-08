// It's Just Pong

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Era/IJPEra.h"
#include "IJPBonusTarget.generated.h"

class AIJPArena;
class AIJPBall;
class UBoxComponent;
class UStaticMeshComponent;

/**
 * A bonus target mid-court (UIJPBonusTargetComponent places them). It blocks the ball like a wall;
 * the ball that hits it breaks it (Break) and bounces off. As a bumper (InitBumper, a boss's
 * pinball skill) it doesn't break: it kicks the ball off faster, until its owner takes it away.
 */
UCLASS()
class IJPONG_API AIJPBonusTarget : public AActor
{
	GENERATED_BODY()

public:
	AIJPBonusTarget();

	/** Place it at plane position Centre, Size across. Call right after spawning. */
	void Init(AIJPArena* InArena, const FVector2D& Centre, float Size) { InitBox(InArena, Centre, FVector2D(Size)); }

	/** As Init, any width and height. */
	void InitBox(AIJPArena* InArena, const FVector2D& Centre, const FVector2D& Size);

	/**
	 * A bumper instead: unbreakable, in the Colour role's colour; a ball bounces off it Boost times
	 * faster (1 = a plain obstacle, like a drifting block).
	 */
	void InitBumper(AIJPArena* InArena, const FVector2D& Centre, const FVector2D& Size, float Boost, EIJPPaletteRole Colour);

	/** Move it (drifting blocks). */
	void SetPlanePosition(const FVector2D& Centre);

	FVector2D GetSize() const { return BoxSize; }

	/** Hit by Ball: a target is gone (and the arena's targets hear about it); a bumper kicks it. */
	void Break(AIJPBall* Ball);

	bool IsBumper() const { return bBumper; }

	FVector2D GetPlanePosition() const { return PlanePosition; }

private:
	UPROPERTY(VisibleAnywhere, Category = "Target")
	TObjectPtr<UBoxComponent> Box;

	UPROPERTY(VisibleAnywhere, Category = "Target")
	TObjectPtr<UStaticMeshComponent> Visual;

	TWeakObjectPtr<AIJPArena> Arena;
	FVector2D PlanePosition = FVector2D::ZeroVector;
	FVector2D BoxSize = FVector2D::ZeroVector;
	bool bBroken = false;
	bool bBumper = false;
	float BumperBoost = 1.f;
};
