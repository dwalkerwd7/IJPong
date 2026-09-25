// It's Just Pong

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "IJPBallType.generated.h"

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
};
