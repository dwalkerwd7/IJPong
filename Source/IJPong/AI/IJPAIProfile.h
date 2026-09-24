// It's Just Pong

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "IJPAIProfile.generated.h"

/** A value that depends on an opponent's Skill (0..1): linear from AtSkill0 to AtSkill1. */
USTRUCT(BlueprintType)
struct FIJPSkillRange
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill")
	float AtSkill0 = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill")
	float AtSkill1 = 0.f;

	float At(float Skill) const { return FMath::Lerp(AtSkill0, AtSkill1, FMath::Clamp(Skill, 0.f, 1.f)); }
};

/**
 * How an AI paddle plays, across the whole difficulty spectrum. There are no difficulty settings in
 * the game: each arena sets its opponent's Skill (0 = hopeless, 1 = near-perfect), and every
 * skill-dependent value here is read at that Skill. The defaults put today's "normal" AI at 0.5.
 * All distances are in arena plane units.
 */
UCLASS(BlueprintType)
class IJPONG_API UIJPAIProfile : public UDataAsset
{
	GENERATED_BODY()

public:
	/** Seconds between re-reading the ball. Also the worst-case delay before reacting to a new shot. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Skill")
	FIJPSkillRange ReactionTime = { 0.25f, 0.05f };

	/**
	 * Largest misjudgement of where the ball will arrive, reached when the ball is at its max speed.
	 * Scales down linearly with ball speed. Rolled once per incoming shot.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Skill")
	FIJPSkillRange ErrorSpread = { 110.f, 10.f };

	/**
	 * How far off-centre it tries to hit, as a fraction of the paddle's reach (1 = its edge).
	 * Off-centre hits send the ball back at an angle, so stronger players aim more. Rolled once per incoming shot.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Skill")
	FIJPSkillRange AimSpread = { 0.2f, 0.8f };

	/** Fraction of the paddle's max speed used while going for the ball. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Skill")
	FIJPSkillRange SpeedScale = { 0.6f, 1.f };

	/** Fraction of the paddle's max speed used to drift back to centre while the ball is heading away. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Skill")
	FIJPSkillRange IdleSpeedScale = { 0.2f, 0.4f };

	/** Within this distance of its target the paddle eases off, so it settles instead of oscillating. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Steering", meta = (ClampMin = "1"))
	float SlowRadius = 20.f;

	/** Close enough to the target to stop moving. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Steering", meta = (ClampMin = "0"))
	float ArrivalTolerance = 2.f;
};
