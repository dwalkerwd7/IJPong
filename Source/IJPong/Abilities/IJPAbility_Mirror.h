// It's Just Pong

#pragma once

#include "CoreMinimal.h"
#include "Abilities/IJPAbility.h"
#include "IJPAbility_Mirror.generated.h"

/**
 * Rival wildcard: watches the other paddle, and when used, makes its own copy of the last ability
 * that paddle used (class skill or run ability) and fires it from this paddle: your Smash comes
 * back at you, your Grow grows the rival. Nothing to copy until you've used something.
 * The copy runs inside this ability (ticks and ball hits are passed on), so a copied armed skill
 * shows the armed cue like any other. The AI uses it as a ball heads its way.
 */
UCLASS()
class IJPONG_API UIJPAbility_Mirror : public UIJPAbility
{
	GENERATED_BODY()

public:
	UIJPAbility_Mirror() { Cooldown = 8.f; Telegraph = 0.5f; }

	virtual bool CanActivate() const override { return LastSeen && !IsActive(); }
	virtual void Activate() override;
	virtual bool IsActive() const override;
	virtual bool IsArmed() const override;
	virtual bool WantsAIUse() const override;
	virtual void OnEquipped() override { Watch(); }
	virtual void TickAbility(float DeltaSeconds) override;
	virtual void OnBallHit(AIJPBall& Ball) override;
	virtual void Deactivate() override;

	/** The last ability the other paddle used (its asset), or null. */
	const UIJPAbility* GetLastSeen() const { return LastSeen; }

	/** The copy made on the last use, or null. */
	const UIJPAbility* GetMirrored() const { return Mirrored; }

private:
	/** Listen to the other paddle's abilities (again, if it changed). */
	void Watch();

	UFUNCTION()
	void HandleOtherActivated(EIJPAbilitySlot Slot, const UIJPAbility* Ability);

	UPROPERTY(Transient)
	TObjectPtr<const UIJPAbility> LastSeen;

	UPROPERTY(Transient)
	TObjectPtr<UIJPAbility> Mirrored;

	TWeakObjectPtr<UIJPAbilityComponent> Watched;
};
