// It's Just Pong

#include "Presentation/IJPScreenShakeComponent.h"
#include "Components/SceneComponent.h"

UIJPScreenShakeComponent::UIJPScreenShakeComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UIJPScreenShakeComponent::Shake(float Seconds, float InAmplitude)
{
	USceneComponent* Moved = Target.Get();
	if (!Moved || Seconds <= 0.f)
	{
		return;
	}
	if (Left <= 0.f)
	{
		Rest = Moved->GetRelativeLocation(); // only when still: a shake on a shake keeps the true rest
	}
	Left = Total = Seconds;
	Strength = InAmplitude > 0.f ? InAmplitude : Amplitude;
	SetComponentTickEnabled(true);
}

void UIJPScreenShakeComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	USceneComponent* Moved = Target.Get();
	if (!Moved)
	{
		SetComponentTickEnabled(false);
		return;
	}
	Left -= DeltaTime;
	if (Left <= 0.f)
	{
		Left = 0.f;
		Moved->SetRelativeLocation(Rest);
		SetComponentTickEnabled(false);
		return;
	}
	// Across the screen only (X and up), never toward the court: the picture shifts, it doesn't zoom.
	const float Fade = Left / FMath::Max(Total, UE_KINDA_SMALL_NUMBER);
	const FVector Jitter(FMath::FRandRange(-1.f, 1.f), 0.f, FMath::FRandRange(-1.f, 1.f));
	Moved->SetRelativeLocation(Rest + Jitter * Strength * Fade);
}
