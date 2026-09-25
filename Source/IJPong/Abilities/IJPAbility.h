// It's Just Pong

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "IJPAbility.generated.h"

class AIJPBall;
class AIJPPaddle;
class UIJPAbilityComponent;

/** A paddle's two ability buttons. */
UENUM(BlueprintType)
enum class EIJPAbilitySlot : uint8
{
	/** Fixed by the paddle's class (its profile). */
	ClassSkill,
	/** Found during a run; any class can use it. */
	RunAbility,
	/** A one-use consumable (Patch, Shield...): spent when used, no cooldown. */
	Item,
	Count UMETA(Hidden)
};

/**
 * One ability. Each subclass is one behaviour (C++); each asset made from a subclass is one tuning
 * of it (Data Asset), so "a stronger Smash" is another asset, not code.
 *
 * The asset itself is never run. Equipping it gives the slot its own copy (DuplicateObject), and
 * the runtime calls below happen on that copy, so it can keep state (armed, time left...) without
 * touching the asset or other paddles using the same asset.
 */
UCLASS(Abstract, BlueprintType)
class IJPONG_API UIJPAbility : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability")
	FText DisplayName;

	/** Seconds before the slot can be used again, counted from activation. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability", meta = (ClampMin = "0", Units = "s"))
	float Cooldown = 5.f;

	/**
	 * Wind-up between the button and the effect: the paddle glows (the armed halo) and a warning
	 * tone plays, so the other side can react. Rival abilities use it; 0 = instant.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability", meta = (ClampMin = "0", Units = "s"))
	float Telegraph = 0.f;

	// --- Runtime, on the equipped copy ---

	/** Called once when equipped. InDefinition = the asset this is a copy of (null = itself). */
	void Init(UIJPAbilityComponent* InOwner, const UIJPAbility* InDefinition = nullptr);

	/** The asset this equipped copy was made from, so others can make copies of their own (Mirror). */
	const UIJPAbility* GetDefinition() const { return Definition ? Definition.Get() : this; }

	/** The ability component this copy is equipped in. */
	UIJPAbilityComponent* GetOwnerComponent() const { return Owner.Get(); }

	/** Whether pressing the button now would do anything (the cooldown is checked separately). */
	virtual bool CanActivate() const { return !IsActive(); }

	virtual void Activate() PURE_VIRTUAL(UIJPAbility::Activate, );

	/** Still doing something: armed, or a timed effect running. */
	virtual bool IsActive() const { return false; }

	/** Waiting to change the paddle's next hit (Smash, Curve shot, Split). The paddle shows a cue while any is. */
	virtual bool IsArmed() const { return false; }

	/**
	 * An AI holding this ability would use it now (asked every frame while the slot is ready).
	 * Abilities written for rivals answer it; the rest never get used by the AI.
	 */
	virtual bool WantsAIUse() const { return false; }

	/** The slot's button was let go (the player's; the AI never holds one). */
	virtual void OnButtonReleased() {}

	/** Just equipped (Init done). */
	virtual void OnEquipped() {}

	/** Every frame while equipped. */
	virtual void TickAbility(float DeltaSeconds) {}

	/** The owning paddle just returned Ball. */
	virtual void OnBallHit(AIJPBall& Ball) {}

	/** Stop any effect right now (unequipped, or the paddle is going away). */
	virtual void Deactivate() {}

	AIJPPaddle* GetPaddle() const;

	/**
	 * Named upgrades on this equipped copy (from the class's skill tree). Each ability reads the
	 * names it knows (e.g. Smash: Power, Curve, ExtraHits, Split); unknown names are ignored.
	 */
	void SetUpgrades(const TMap<FName, float>& InUpgrades) { Upgrades = InUpgrades; }

	/** An upgrade's value, 0 if not set. */
	float GetUpgrade(FName Name) const { return Upgrades.FindRef(Name); }

private:
	TMap<FName, float> Upgrades;

	TWeakObjectPtr<UIJPAbilityComponent> Owner;

	UPROPERTY(Transient)
	TObjectPtr<const UIJPAbility> Definition;
};
