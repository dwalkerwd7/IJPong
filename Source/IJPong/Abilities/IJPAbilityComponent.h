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

	/** Multiply the slot's cooldowns from now on (0.85 = 15% shorter), e.g. from a run's modifiers. */
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void SetCooldownScale(EIJPAbilitySlot Slot, float Scale);

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

	/** Any equipped ability is armed, waiting for the paddle's next hit. */
	UFUNCTION(BlueprintPure, Category = "Abilities")
	bool IsArmed() const;

	/** The slot's ability was triggered and is winding up (its Telegraph) before it takes effect. */
	UFUNCTION(BlueprintPure, Category = "Abilities")
	bool IsWindingUp(EIJPAbilitySlot Slot) const;

	/** The paddle's glow should show: something is armed or winding up. */
	bool ShouldShowCue() const;

	/** The owning paddle just returned Ball. */
	void HandleBallHit(AIJPBall& Ball);

	AIJPPaddle* GetPaddle() const;

	UPROPERTY(BlueprintAssignable, Category = "Abilities")
	FIJPAbilityActivatedSignature OnActivated;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** The rising two-note chirp that confirms a skill is armed. */
	void PlayArmChirp();
	void PlayWarning();
	/** Run the slot's ability now (after any wind-up). */
	void Fire(int32 Index);

	/** One entry per EIJPAbilitySlot. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UIJPAbility>> Abilities;

	TArray<float> Cooldowns;
	/** Time left in each slot's wind-up (0 = none). */
	TArray<float> WindUps;
	TArray<float> CooldownScales;
	FTimerHandle ChirpTimer;
};
