// It's Just Pong

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Core/IJPTypes.h"
#include "IJPCourtShiftComponent.generated.h"

class AIJPArena;
class AIJPBall;

/**
 * The court reshaping between points (the era's FIJPCourtShift, the HD twist). When a goal empties
 * the court, a new size is picked between MinScale and MaxScale and the arena eases to it over
 * ShiftTime (AIJPArena::SetCourtScale). A new match, or an era without it, is back to full size.
 */
UCLASS(ClassGroup = (IJPong))
class IJPONG_API UIJPCourtShiftComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UIJPCourtShiftComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Full size now, nothing in progress. */
	void Reset();

	/** Start easing to Scale (tests, debugging). */
	void ShiftTo(const FVector2D& Scale);

	bool IsShifting() const { return ShiftLeft > 0.f; }

private:
	UFUNCTION()
	void HandleGoal(AIJPBall* Ball, EIJPSide DefendingSide);

	AIJPArena* GetArena() const;

	FVector2D From = FVector2D(1.f, 1.f);
	FVector2D To = FVector2D(1.f, 1.f);
	float ShiftLeft = 0.f;
	float ShiftTotal = 0.f;
};
