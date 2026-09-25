// It's Just Pong

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "IJPBallType.generated.h"

class UTexture2D;

/**
 * What kind of ball this is: its size, how fast it goes and what it's worth. The defaults are
 * the standard 1972 ball. New ball types are new assets, not code.
 */
UCLASS(BlueprintType)
class IJPONG_API UIJPBallType : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball")
	FText DisplayName;

	/** Edge length of the square ball. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball|Layout", meta = (ClampMin = "1"))
	float Size = 10.f;

	/** Speed when served. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball|Movement", meta = (ClampMin = "0"))
	float BaseSpeed = 400.f;

	/** Added to the speed on every paddle hit, up to MaxSpeed. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball|Movement", meta = (ClampMin = "0"))
	float SpeedPerHit = 25.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball|Movement", meta = (ClampMin = "0"))
	float MaxSpeed = 1000.f;

	/** Points the other side scores when this ball goes into a goal. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball|Scoring", meta = (ClampMin = "1"))
	int32 Points = 1;

	/** This type's colour cue, shown only in eras whose palette turns on bBallTypeColours. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball|Look")
	FLinearColor Colour = FLinearColor::White;

	/** The ball's sprite (greyscale, tinted by its colour), shown in eras with sprites. Empty = a plain square. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball|Look")
	TObjectPtr<UTexture2D> Sprite;

	/** An early, square version used by eras that ask for classic balls. Empty = Sprite. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball|Look")
	TObjectPtr<UTexture2D> ClassicSprite;

	// --- Behaviours (any mix; all off by default) ---

	/**
	 * Ghost: hidden while in the middle of the court, as a fraction of the half-width from the net
	 * (0.4 = the middle 40% each side of the net). It still moves and bounces; you just can't see it.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball|Behaviour", meta = (ClampMin = "0", ClampMax = "1"))
	float GhostBand = 0.f;

	/** Twin: its first paddle return fans it into two balls of this type (neither splits again). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball|Behaviour")
	bool bSplitsOnFirstHit = false;

	/** Degrees between the two halves of a Twin's split. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball|Behaviour", meta = (ClampMin = "0", ClampMax = "60", EditCondition = "bSplitsOnFirstHit"))
	float SplitSpread = 20.f;

	/** Bomb: a goal with it stuns the paddle it got past for this long (can't move). 0 = none. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball|Behaviour", meta = (ClampMin = "0", Units = "s"))
	float StunOnGoal = 0.f;

	/** Leech: a goal with it also heals whoever scored by this much. 0 = none. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball|Behaviour", meta = (ClampMin = "0"))
	float HealOnGoal = 0.f;
};
