// It's Just Pong

#pragma once

#include "CoreMinimal.h"
#include "Abilities/IJPAbility.h"
#include "IJPAbility_Warp.generated.h"

class AIJPBall;

/**
 * Rival wildcard: the ball heading at the other side jumps to the mirrored height (above the middle
 * becomes as far below), keeping its speed and angle, so they have to re-read it. Near the middle it
 * jumps at least MinJump instead. The AI warps a ball as it crosses the net toward them.
 */
UCLASS()
class IJPONG_API UIJPAbility_Warp : public UIJPAbility
{
	GENERATED_BODY()

public:
	UIJPAbility_Warp() { Cooldown = 8.f; Telegraph = 0.5f; }

	/** The smallest jump, for a ball close to the middle (where mirroring would barely move it). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Warp", meta = (ClampMin = "0"))
	float MinJump = 120.f;

	/** The AI warps a ball once it's this far past the net toward them (and not further than twice this). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Warp", meta = (ClampMin = "0"))
	float AIPastNet = 20.f;

	virtual void Activate() override;
	virtual bool WantsAIUse() const override;

private:
	AIJPPaddle* GetTarget() const;
	/** The ball heading at the other side that's furthest from them, or null. */
	AIJPBall* PickBall() const;
};
