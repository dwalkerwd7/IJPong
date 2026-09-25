// It's Just Pong

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "IJPMatchRules.generated.h"

class UIJPBallType;

/**
 * How one match is played and won. Both sides start with health; a goal takes some away (as do
 * spells, later), and the side left with none loses. Different matches (a quick fight, a long
 * one) are different assets, not code.
 */
UCLASS(BlueprintType)
class IJPONG_API UIJPMatchRules : public UDataAsset
{
	GENERATED_BODY()

public:
	/**
	 * Each side's health at the start (a run sets the player's from the run's instead).
	 * 0 = endless: no health, nobody ever wins, and the numbers show goals like 1972.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Match|Health", meta = (ClampMin = "0"))
	float StartingHealth = 5.f;

	/** Health a goal takes, per point the ball is worth (a Heavy ball's 2 points = twice this). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Match|Health", meta = (ClampMin = "0"))
	float GoalDamage = 1.f;

	/** Pause before each serve, including the first. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Match|Serve", meta = (ClampMin = "0", Units = "s"))
	float ServeDelay = 1.f;

	/** Serves leave at a random angle within +-this from horizontal. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Match|Serve", meta = (ClampMin = "0", Units = "deg"))
	float MaxServeAngleDeg = 30.f;

	/**
	 * The balls launched at every serve, alternating direction (the first goes to the side receiving).
	 * Empty = one ball of the arena's default type. An empty entry also means the default type.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Match|Serve")
	TArray<TObjectPtr<UIJPBallType>> ServedBalls;

	bool IsEndless() const { return StartingHealth <= 0.f; }
};
