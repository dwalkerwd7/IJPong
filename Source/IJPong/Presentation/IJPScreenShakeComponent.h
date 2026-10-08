// It's Just Pong

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "IJPScreenShakeComponent.generated.h"

class USceneComponent;

/** Shakes a camera (the arena's) for a moment: a jitter that fades out, then back where it was. */
UCLASS(ClassGroup = (IJPong))
class IJPONG_API UIJPScreenShakeComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UIJPScreenShakeComponent();

	/** What moves; set once by the owner. */
	void SetTarget(USceneComponent* InTarget) { Target = InTarget; }

	/** Jitter by up to Amplitude units, fading over Seconds. */
	void Shake(float Seconds, float InAmplitude = 0.f);

	bool IsShaking() const { return Left > 0.f; }

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Default jitter, in arena units. */
	UPROPERTY(EditAnywhere, Category = "Shake", meta = (ClampMin = "0"))
	float Amplitude = 8.f;

private:
	TWeakObjectPtr<USceneComponent> Target;
	FVector Rest = FVector::ZeroVector;
	float Left = 0.f;
	float Total = 0.f;
	float Strength = 0.f;
};
