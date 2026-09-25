// It's Just Pong

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Run/IJPRunMap.h"
#include "IJPActConfig.generated.h"

class UIJPMatchRules;
class UIJPReward;
class UIJPRival;

/** Who and how you play at one kind of fight node. */
USTRUCT(BlueprintType)
struct FIJPEncounter
{
	GENERATED_BODY()

	/** One is picked at random for each node. Empty = no rival (the arena's own opponent). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Encounter")
	TArray<TObjectPtr<UIJPRival>> Rivals;

	/** Empty = UIJPMatchRules' defaults. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Encounter")
	TObjectPtr<UIJPMatchRules> Rules;

	/** How well the opponent plays here: 0 hopeless .. 1 near-perfect (the arena's OpponentSkill). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Encounter", meta = (ClampMin = "0", ClampMax = "1"))
	float Skill = 0.5f;

	/** Paid for winning. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Encounter", meta = (ClampMin = "0"))
	int32 Coins = 10;
};

/**
 * One act of a run: the shape of its map, how often each node type turns up, and what each fight
 * node pits you against. Different acts are different assets.
 */
UCLASS(BlueprintType)
class IJPONG_API UIJPActConfig : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Act")
	FText DisplayName;

	// --- Map shape ---

	/** Rows from top to bottom, counting the boss's row. The row just above the boss is always Rest. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Act|Map", meta = (ClampMin = "3", ClampMax = "12"))
	int32 Rows = 7;

	/** Side-by-side lanes the paths wander between. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Act|Map", meta = (ClampMin = "1", ClampMax = "6"))
	int32 Lanes = 4;

	/** Paths walked from start to boss; more paths = more branching. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Act|Map", meta = (ClampMin = "1", ClampMax = "6"))
	int32 Paths = 3;

	/** Relative chances of each type in the middle rows (the top row is always Match). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Act|Map", meta = (ClampMin = "0"))
	float MatchWeight = 6.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Act|Map", meta = (ClampMin = "0"))
	float EliteWeight = 2.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Act|Map", meta = (ClampMin = "0"))
	float RestWeight = 1.5f;

	/** Elites can't appear above this row (0 = top). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Act|Map", meta = (ClampMin = "1"))
	int32 EliteFromRow = 2;

	// --- Nodes ---

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Act|Nodes")
	FIJPEncounter Match;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Act|Nodes")
	FIJPEncounter Elite;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Act|Nodes")
	FIJPEncounter Boss;

	// --- Rewards ---

	/** Offered (a few at random) after winning a Match. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Act|Rewards")
	TArray<TObjectPtr<UIJPReward>> MatchRewards;

	/** Offered after winning an Elite. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Act|Rewards")
	TArray<TObjectPtr<UIJPReward>> EliteRewards;

	/** How many rewards each pick offers. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Act|Rewards", meta = (ClampMin = "1", ClampMax = "4"))
	int32 RewardChoices = 3;

	/** Coins for skipping a pick instead. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Act|Rewards", meta = (ClampMin = "0"))
	int32 SkipCoins = 5;

	/** The reward pool after winning a node of this type (empty for the boss: the act ends). */
	const TArray<TObjectPtr<UIJPReward>>* GetRewardPool(EIJPNodeType Type) const
	{
		return Type == EIJPNodeType::Match ? &MatchRewards : Type == EIJPNodeType::Elite ? &EliteRewards : nullptr;
	}

	/** Health restored at a Rest node (never above the run's maximum). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Act|Nodes", meta = (ClampMin = "0"))
	int32 RestHeal = 3;

	/** The encounter for a fight node type, or null for Rest (and the unbuilt Shop / Event). */
	const FIJPEncounter* GetEncounter(EIJPNodeType Type) const
	{
		switch (Type)
		{
		case EIJPNodeType::Match: return &Match;
		case EIJPNodeType::Elite: return &Elite;
		case EIJPNodeType::Boss:  return &Boss;
		default:                  return nullptr;
		}
	}
};
