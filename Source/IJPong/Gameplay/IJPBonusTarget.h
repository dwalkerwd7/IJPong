// It's Just Pong

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "IJPBonusTarget.generated.h"

class AIJPArena;
class AIJPBall;
class UBoxComponent;
class UStaticMeshComponent;

/**
 * A bonus target mid-court (UIJPBonusTargetComponent places them). It blocks the ball like a wall;
 * the ball that hits it breaks it (Break) and bounces off.
 */
UCLASS()
class IJPONG_API AIJPBonusTarget : public AActor
{
	GENERATED_BODY()

public:
	AIJPBonusTarget();

	/** Place it at plane position Centre, Size across. Call right after spawning. */
	void Init(AIJPArena* InArena, const FVector2D& Centre, float Size);

	/** Hit by Ball: it's gone, and the arena's targets hear about it. */
	void Break(AIJPBall* Ball);

	FVector2D GetPlanePosition() const { return PlanePosition; }

private:
	UPROPERTY(VisibleAnywhere, Category = "Target")
	TObjectPtr<UBoxComponent> Box;

	UPROPERTY(VisibleAnywhere, Category = "Target")
	TObjectPtr<UStaticMeshComponent> Visual;

	TWeakObjectPtr<AIJPArena> Arena;
	FVector2D PlanePosition = FVector2D::ZeroVector;
	bool bBroken = false;
};
