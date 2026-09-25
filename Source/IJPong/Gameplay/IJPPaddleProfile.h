// It's Just Pong

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "IJPPaddleProfile.generated.h"

/**
 * A paddle's base stats: its size and how it moves. A paddle class (UIJPPaddleClass) picks one.
 * All distances are in arena plane units.
 */
UCLASS(BlueprintType)
class IJPONG_API UIJPPaddleProfile : public UDataAsset
{
	GENERATED_BODY()

public:
	/** Width (plane X, toward the goals) and length (plane Y, along the lane). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paddle|Layout", meta = (ClampMin = "1"))
	FVector2D Size = FVector2D(10.f, 76.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paddle|Movement", meta = (ClampMin = "0"))
	float MaxSpeed = 700.f;

	/** Time to go from rest to MaxSpeed (and back to rest). 0 = fully instant. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paddle|Movement", meta = (ClampMin = "0", Units = "s"))
	float RampTime = 0.05f;

	/** How long the paddle blinks off when the ball hits it. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paddle|Presentation", meta = (ClampMin = "0.01", Units = "s"))
	float FlickerTime = 0.05f;
};
