// It's Just Pong

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Abilities/IJPAbility.h"
#include "IJPAbilityComponent.generated.h"

class AIJPBall;
class AIJPPaddle;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FIJPAbilityActivatedSignature, EIJPAbilitySlot, Slot, const UIJPAbility*, Ability);

/**
 * A paddle's ability slots: what's equipped in each, and each slot's cooldown. The player
 * controller (and later the AI) presses the buttons through TryActivate; the arena tells it
 * when its paddle returns a ball.
 */
UCLASS(ClassGroup = (IJPong), meta = (BlueprintSpawnableComponent))
class IJPONG_API UIJPAbilityComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UIJPAbilityComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Put an ability in a slot (its own copy of Definition), replacing and stopping what was there. Null empties the slot. */
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void Equip(EIJPAbilitySlot Slot, const UIJPAbility* Definition);

	/** Press the slot's button. True if the ability activated. */
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	bool TryActivate(EIJPAbilitySlot Slot);

	/** The slot's equipped copy, or null. */
	UFUNCTION(BlueprintPure, Category = "Abilities")
	UIJPAbility* GetAbility(EIJPAbilitySlot Slot) const;

	UFUNCTION(BlueprintPure, Category = "Abilities")
	float GetCooldownRemaining(EIJPAbilitySlot Slot) const;

	/** Equipped, off cooldown, and able to activate. */
	UFUNCTION(BlueprintPure, Category = "Abilities")
	bool IsReady(EIJPAbilitySlot Slot) const;

	/** The owning paddle just returned Ball. */
	void HandleBallHit(AIJPBall& Ball);

	AIJPPaddle* GetPaddle() const;

	UPROPERTY(BlueprintAssignable, Category = "Abilities")
	FIJPAbilityActivatedSignature OnActivated;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** One entry per EIJPAbilitySlot. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UIJPAbility>> Abilities;

	TArray<float> Cooldowns;
};
