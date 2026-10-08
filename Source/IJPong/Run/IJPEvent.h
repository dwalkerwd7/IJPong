// It's Just Pong

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Run/IJPActConfig.h"
#include "IJPEvent.generated.h"

class UIJPReward;
class UIJPRunSubsystem;

/** What happens to the run when an event's option plays out one way. */
USTRUCT(BlueprintType)
struct FIJPEventOutcome
{
	GENERATED_BODY()

	/** Shown on the result card. Line breaks are kept (text doesn't wrap). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Event", meta = (MultiLine = "true"))
	FText Text;

	/** Coins gained (negative = paid). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Event")
	int32 Coins = 0;

	/** Health gained (negative = lost; never below 1 from an event). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Event")
	float Health = 0.f;

	/** Raises the run's maximum health (and heals as much). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Event", meta = (ClampMin = "0"))
	float MaxHealth = 0.f;

	/** Given to the run, like a picked reward card. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Event")
	TObjectPtr<UIJPReward> Reward;

	/** Apply it to the run. */
	void Apply(UIJPRunSubsystem& Run) const;

	/** A short line of what it does, for cards ("+15 COINS  -1 HP"). Empty if nothing. */
	FString Summary() const;
};

/** One choice in an event: a card to pick. */
USTRUCT(BlueprintType)
struct FIJPEventOption
{
	GENERATED_BODY()

	/** The card's title. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Event")
	FText Label;

	/** The card's text. Empty = a summary of the success outcome (with the chance, if any). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Event", meta = (MultiLine = "true"))
	FText Description;

	/** Chance of Success; otherwise Failure. 1 = a sure thing. Ignored for a match. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Event", meta = (ClampMin = "0", ClampMax = "1", EditCondition = "!bMatch"))
	float Chance = 1.f;

	/**
	 * A match with a twist first: Success if you win it, Failure if you lose. The twist is the match's
	 * (its rules: the balls served, the health), never your paddle's.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Event")
	bool bMatch = false;

	/** Who and how: Rivals empty = one of the act's Match rivals. Its Coins are ignored (pay in Success). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Event", meta = (EditCondition = "bMatch"))
	FIJPEncounter Match;

	/** Your health in that match (e.g. 1 for sudden death); 0 = the run's, as in any fight. What you lose still comes off the run. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Event", meta = (ClampMin = "0", EditCondition = "bMatch"))
	float PlayerHealth = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Event")
	FIJPEventOutcome Success;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Event", meta = (EditCondition = "Chance < 1 || bMatch"))
	FIJPEventOutcome Failure;

	/** What a sure cost asks of the run up front (a paid option needs the coins). */
	bool CanChoose(const UIJPRunSubsystem& Run) const;

	/** The card's text: Description, or a summary. */
	FString CardText() const;
};

/**
 * An Event node's content: a short text and a few options, each a card. Risk against reward:
 * options trade coins and health for rewards, some on a chance. Placed by an act's Events pool.
 */
UCLASS(BlueprintType)
class IJPONG_API UIJPEvent : public UDataAsset
{
	GENERATED_BODY()

public:
	/** The screen's heading. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Event")
	FText Title;

	/** Typed out above the cards. Line breaks are kept (text doesn't wrap). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Event", meta = (MultiLine = "true"))
	FText Text;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Event")
	TArray<FIJPEventOption> Options;
};
