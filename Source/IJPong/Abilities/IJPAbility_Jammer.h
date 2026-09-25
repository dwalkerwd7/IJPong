// It's Just Pong

#pragma once

#include "CoreMinimal.h"
#include "Abilities/IJPAbility.h"
#include "IJPAbility_Jammer.generated.h"

/**
 * Rival wildcard: jams the other side for a moment: static fills the screen (the arena's CRT noise
 * rises) and their class skill is locked (UIJPAbilityComponent::LockSlot). Their run ability and
 * anything already running still work. The AI jams when a ball is heading at them and their class
 * skill is ready, so it's taken away just when they'd want it.
 */
UCLASS()
class IJPONG_API UIJPAbility_Jammer : public UIJPAbility
{
	GENERATED_BODY()

public:
	UIJPAbility_Jammer() { Cooldown = 10.f; Telegraph = 0.5f; }

	/** How long their class skill is locked, and the static lasts. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Jammer", meta = (ClampMin = "0", Units = "s"))
	float Duration = 2.f;

	/** Static added to the screen's noise (the Cabinet tube's own is 0.25). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Jammer", meta = (ClampMin = "0"))
	float StaticStrength = 0.6f;

	virtual void Activate() override;
	virtual bool WantsAIUse() const override;

private:
	AIJPPaddle* GetTarget() const;
};
