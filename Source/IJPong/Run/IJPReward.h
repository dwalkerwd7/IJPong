// It's Just Pong

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "IJPReward.generated.h"

class UIJPAbility;
class UIJPBallType;
class UIJPRunSubsystem;

/**
 * Something the player can pick after winning a fight. Each subclass is one kind of reward;
 * each asset is one offer (its name, its blurb, and what it gives). It changes the run's loadout
 * (UIJPRunSubsystem), which the run applies to every fight from then on.
 */
UCLASS(Abstract, BlueprintType)
class IJPONG_API UIJPReward : public UDataAsset
{
	GENERATED_BODY()

public:
	/** Card title. Keep it short: cards are narrow. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward")
	FText DisplayName;

	/** Card text under the title. Line breaks are kept (cards don't wrap). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward", meta = (MultiLine = "true"))
	FText Description;

	/** Worth offering right now (e.g. not the run ability you already have). */
	virtual bool CanOffer(const UIJPRunSubsystem& Run) const { return true; }

	/** Give it to the run. */
	virtual void Grant(UIJPRunSubsystem& Run) const PURE_VIRTUAL(UIJPReward::Grant, );
};

/** What a modifier improves, for the rest of the run. */
UENUM(BlueprintType)
enum class EIJPRunStat : uint8
{
	/** Amount = fraction added to the paddle's length (0.1 = +10%). */
	PaddleLength,
	/** Amount = fraction added to the paddle's top speed. */
	PaddleSpeed,
	/** Amount = extra max health (also healed). */
	MaxHealth,
	/** Amount = fraction cut from the class skill's cooldown (0.15 = 15% shorter). */
	ClassSkillCooldown
};

/** A lasting boost for the run. Picking the same one again stacks. */
UCLASS()
class IJPONG_API UIJPReward_Modifier : public UIJPReward
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Modifier")
	EIJPRunStat Stat = EIJPRunStat::PaddleLength;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Modifier")
	float Amount = 0.1f;

	virtual void Grant(UIJPRunSubsystem& Run) const override;
};

/** A run ability for the second slot, replacing the one held (if any). */
UCLASS()
class IJPONG_API UIJPReward_Ability : public UIJPReward
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability")
	TObjectPtr<UIJPAbility> Ability;

	virtual bool CanOffer(const UIJPRunSubsystem& Run) const override;
	virtual void Grant(UIJPRunSubsystem& Run) const override;
};

/** From now on every serve also launches a ball of this type. Picking it again adds another. */
UCLASS()
class IJPONG_API UIJPReward_Ball : public UIJPReward
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball")
	TObjectPtr<UIJPBallType> BallType;

	virtual void Grant(UIJPRunSubsystem& Run) const override;
};
