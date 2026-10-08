// It's Just Pong

#pragma once

#include "CoreMinimal.h"
#include "Abilities/IJPAbility.h"
#include "IJPAbility_Split.generated.h"

class AIJPArena;
class UIJPBallType;

/**
 * Arm the paddle: its next return fans out into two balls of the same type.
 * Skill-tree upgrades: Spread (degrees added), Triple (> 0: a third ball down the middle), Smash
 * (> 0: every piece leaves boosted), Decoy (> 0: the new ball is a DecoyType, e.g. a Ghost).
 */
UCLASS()
class IJPONG_API UIJPAbility_Split : public UIJPAbility
{
	GENERATED_BODY()

public:
	UIJPAbility_Split() { Cooldown = 8.f; }

	/** Angle between the two balls' paths. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Split", meta = (ClampMin = "0", ClampMax = "90", Units = "deg"))
	float SpreadDeg = 30.f;

	/** Split Smash: each piece leaves this many times faster. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Split|Upgrades", meta = (ClampMin = "1"))
	float SmashBoost = 1.3f;

	/** Decoy: the new half becomes this type (a Ghost hides near the net). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Split|Upgrades")
	TObjectPtr<UIJPBallType> DecoyType;

	/** Fan Ball's path out by Spread degrees: it turns half one way, a new ball of its type goes half the other. */
	static AIJPBall* FanOut(AIJPBall& Ball, AIJPArena& Arena, float Spread);

	virtual void Activate() override { bArmed = true; }
	virtual bool IsActive() const override { return bArmed; }
	virtual bool IsArmed() const override { return bArmed; }
	virtual void OnBallHit(AIJPBall& Ball) override;
	virtual void Deactivate() override { bArmed = false; }

private:
	bool bArmed = false;
};
