// It's Just Pong

#pragma once

#include "CoreMinimal.h"
#include "Components/BoxComponent.h"
#include "Core/IJPTypes.h"
#include "IJPGoalComponent.generated.h"

/** A box behind a paddle. When the ball's sweep hits it, DefendingSide concedes a point. */
UCLASS(ClassGroup = (IJPong), meta = (BlueprintSpawnableComponent))
class IJPONG_API UIJPGoalComponent : public UBoxComponent
{
	GENERATED_BODY()

public:
	UIJPGoalComponent();

	/** The side that loses a point when the ball enters this goal. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Goal")
	EIJPSide DefendingSide = EIJPSide::Left;
};
