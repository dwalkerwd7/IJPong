// It's Just Pong

#pragma once

#include "CoreMinimal.h"
#include "Abilities/IJPAbility.h"
#include "Core/IJPTypes.h"
#include "IJPItem_Shield.generated.h"

class AIJPBall;

/**
 * Item: the next goal against you is blocked. Raises your side's barrier (the Bulwark's line) until
 * one ball bounces off it, then drops it. A Breaker's piercing shot goes straight through it.
 */
UCLASS()
class IJPONG_API UIJPItem_Shield : public UIJPAbility
{
	GENERATED_BODY()

public:
	UIJPItem_Shield() { Cooldown = 0.f; }

	virtual void Activate() override;
	virtual bool IsActive() const override { return bUp; }
	virtual void Deactivate() override { Lower(); }

private:
	void Lower();

	UFUNCTION()
	void HandleBarrierHit(EIJPSide Side, AIJPBall* Ball);

	bool bUp = false;
};
