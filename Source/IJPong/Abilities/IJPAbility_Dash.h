// It's Just Pong

#pragma once

#include "CoreMinimal.h"
#include "Abilities/IJPAbility.h"
#include "IJPAbility_Dash.generated.h"

/**
 * An instant burst of movement the way the paddle is being steered (AIJPPaddle::Dash).
 * Skill-tree upgrades: Distance (fraction added to the distance), ExtraDashes (free dashes before
 * the cooldown starts), Slipstream (> 0: a speed boost after each dash), DashStrike (> 0: a return
 * made during or just after a dash is smashed).
 */
UCLASS()
class IJPONG_API UIJPAbility_Dash : public UIJPAbility
{
	GENERATED_BODY()

public:
	UIJPAbility_Dash() { Cooldown = 3.f; }

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dash", meta = (ClampMin = "0"))
	float Distance = 120.f;

	/** How long the burst takes. Short = closer to a teleport. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dash", meta = (ClampMin = "0.01", Units = "s"))
	float Duration = 0.08f;

	/** Slipstream: top speed times this after a dash... */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dash|Upgrades", meta = (ClampMin = "1"))
	float SlipstreamSpeed = 1.3f;

	/** ...for this long. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dash|Upgrades", meta = (ClampMin = "0", Units = "s"))
	float SlipstreamTime = 1.f;

	/** Dash Strike: a return within this long of a dash starting... */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dash|Upgrades", meta = (ClampMin = "0", Units = "s"))
	float StrikeWindow = 0.3f;

	/** ...leaves this many times faster (a Smash boost). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dash|Upgrades", meta = (ClampMin = "1"))
	float StrikeBoost = 1.3f;

	virtual void Activate() override;
	virtual bool StartsCooldown() override;
	virtual void TickAbility(float DeltaSeconds) override;
	virtual void OnBallHit(AIJPBall& Ball) override;

private:
	/** Dashes taken since the cooldown last started. */
	int32 DashesInCycle = 0;
	float SinceDash = TNumericLimits<float>::Max();
};
