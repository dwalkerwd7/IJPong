// It's Just Pong

#include "Gameplay/IJPCourtShiftComponent.h"
#include "Era/IJPEra.h"
#include "Era/IJPEraSubsystem.h"
#include "Gameplay/IJPArena.h"
#include "Gameplay/IJPBall.h"

UIJPCourtShiftComponent::UIJPCourtShiftComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UIJPCourtShiftComponent::BeginPlay()
{
	Super::BeginPlay();
	if (AIJPArena* Arena = GetArena())
	{
		Arena->OnBallGoal.AddDynamic(this, &UIJPCourtShiftComponent::HandleGoal);
	}
}

void UIJPCourtShiftComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	AIJPArena* Arena = GetArena();
	if (!Arena)
	{
		return;
	}
	if (!Arena->IsTwistOn(EIJPTwist::CourtShift) && !Arena->GetCourtScale().Equals(FVector2D(1.f, 1.f)))
	{
		Reset();
		return;
	}
	if (ShiftLeft <= 0.f)
	{
		return;
	}
	ShiftLeft = FMath::Max(ShiftLeft - DeltaTime, 0.f);
	const float Alpha = 1.f - ShiftLeft / FMath::Max(ShiftTotal, UE_KINDA_SMALL_NUMBER);
	Arena->SetCourtScale(FMath::Lerp(From, To, FMath::InterpEaseInOut(0.f, 1.f, Alpha, 2.f)));
}

void UIJPCourtShiftComponent::Reset()
{
	ShiftLeft = 0.f;
	From = To = FVector2D(1.f, 1.f);
	if (AIJPArena* Arena = GetArena())
	{
		Arena->SetCourtScale(FVector2D(1.f, 1.f));
	}
}

void UIJPCourtShiftComponent::ShiftTo(const FVector2D& Scale)
{
	const AIJPArena* Arena = GetArena();
	const UIJPEra* Era = UIJPEraSubsystem::GetCurrentEra(this);
	From = Arena ? Arena->GetCourtScale() : FVector2D(1.f, 1.f);
	To = Scale;
	ShiftTotal = ShiftLeft = FMath::Max(Era ? Era->CourtShift.ShiftTime : 0.6f, UE_KINDA_SMALL_NUMBER);
}

void UIJPCourtShiftComponent::HandleGoal(AIJPBall* Ball, EIJPSide DefendingSide)
{
	// Only between points: once the last ball is out, before the next serve.
	const AIJPArena* Arena = GetArena();
	const UIJPEra* Era = UIJPEraSubsystem::GetCurrentEra(this);
	if (!Arena || !Era || !Arena->IsTwistOn(EIJPTwist::CourtShift) || Arena->GetNumBallsInPlay() > 0)
	{
		return;
	}
	const FIJPCourtShift& Settings = Era->CourtShift;
	ShiftTo(FVector2D(FMath::FRandRange(Settings.MinScale.X, Settings.MaxScale.X), FMath::FRandRange(Settings.MinScale.Y, Settings.MaxScale.Y)));
}

AIJPArena* UIJPCourtShiftComponent::GetArena() const
{
	return Cast<AIJPArena>(GetOwner());
}
