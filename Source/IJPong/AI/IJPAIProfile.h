// It's Just Pong

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "IJPAIProfile.generated.h"

/**
 * How an AI paddle plays. Difficulty presets are just different assets.
 * All distances are in arena plane units.
 */
UCLASS(BlueprintType)
class IJPONG_API UIJPAIProfile : public UDataAsset
{
	GENERATED_BODY()

public:
	/** Seconds between re-reading the ball. Also the worst-case delay before reacting to a new shot. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI", meta = (ClampMin = "0", Units = "s"))
	float ReactionTime = 0.15f;

	/**
	 * Largest misjudgement of where the ball will arrive, reached when the ball is at its max speed.
	 * Scales down linearly with ball speed. Rolled once per incoming shot.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI", meta = (ClampMin = "0"))
	float ErrorSpread = 60.f;

	/**
	 * How far off-centre it tries to hit, as a fraction of the paddle's reach (1 = its edge).
	 * Off-centre hits send the ball back at an angle. Rolled once per incoming shot.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI", meta = (ClampMin = "0", ClampMax = "1"))
	float AimSpread = 0.5f;

	/** Fraction of the paddle's max speed used while going for the ball. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI", meta = (ClampMin = "0", ClampMax = "1"))
	float SpeedScale = 0.85f;

	/** Fraction of the paddle's max speed used to drift back to centre while the ball is heading away. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI", meta = (ClampMin = "0", ClampMax = "1"))
	float IdleSpeedScale = 0.3f;

	/** Within this distance of its target the paddle eases off, so it settles instead of oscillating. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI", meta = (ClampMin = "1"))
	float SlowRadius = 20.f;

	/** Close enough to the target to stop moving. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI", meta = (ClampMin = "0"))
	float ArrivalTolerance = 2.f;
};
