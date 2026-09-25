// It's Just Pong

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "IJPMatchRules.generated.h"

class UIJPBallType;

/**
 * How one match is played and won. Different matches (a quick first-to-3, a long first-to-7)
 * are different assets, not code.
 */
UCLASS(BlueprintType)
class IJPONG_API UIJPMatchRules : public UDataAsset
{
	GENERATED_BODY()

public:
	/** Points needed to win the match. 0 = endless: nobody ever wins. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Match|Scoring", meta = (ClampMin = "0"))
	int32 WinTarget = 5;

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

	bool IsEndless() const { return WinTarget <= 0; }
};
