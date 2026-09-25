// It's Just Pong

#pragma once

#include "CoreMinimal.h"
#include "Abilities/IJPAbility.h"
#include "IJPAbility_Breaker.generated.h"

/**
 * Rival ability: arm the paddle, and its next return cracks through barriers (AIJPBall::SetPiercing).
 * Only that one ball: the barrier stays up and stops everything else, so it's harder for the
 * Bulwark but never a wall it can't hold. The AI arms it for an incoming ball when the other side
 * has a barrier up or can raise one.
 */
UCLASS()
class IJPONG_API UIJPAbility_Breaker : public UIJPAbility
{
	GENERATED_BODY()

public:
	UIJPAbility_Breaker() { Cooldown = 8.f; Telegraph = 0.5f; }

	virtual void Activate() override { bArmed = true; }
	virtual bool IsActive() const override { return bArmed; }
	virtual bool IsArmed() const override { return bArmed; }
	virtual bool WantsAIUse() const override;
	virtual void OnBallHit(AIJPBall& Ball) override;
	virtual void Deactivate() override { bArmed = false; }

private:
	bool bArmed = false;
};
