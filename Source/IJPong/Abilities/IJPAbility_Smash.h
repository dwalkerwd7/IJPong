// It's Just Pong

#pragma once

#include "CoreMinimal.h"
#include "Abilities/IJPAbility.h"
#include "IJPAbility_Smash.generated.h"

/**
 * Arm the paddle: its next return leaves much faster. One shot only: the hit after that goes
 * back to the rally's normal speed (see AIJPBall::Boost).
 * Skill-tree upgrades it reads: Power (added to the multiplier), Curve (the smash bends),
 * ExtraHits (that many more returns are smashed per activation), Split (the smash fans into two).
 */
UCLASS()
class IJPONG_API UIJPAbility_Smash : public UIJPAbility
{
	GENERATED_BODY()

public:
	UIJPAbility_Smash() { Cooldown = 6.f; }

	/** The smashed return's speed, as a multiple of what it would have been. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Smash", meta = (ClampMin = "1"))
	float SpeedMultiplier = 1.6f;

	virtual void Activate() override;
	virtual bool IsActive() const override { return HitsLeft > 0; }
	virtual bool IsArmed() const override { return HitsLeft > 0; }
	virtual void OnBallHit(AIJPBall& Ball) override;
	virtual void Deactivate() override { HitsLeft = 0; }

private:
	/** Returns still to be smashed on this activation. */
	int32 HitsLeft = 0;
};
