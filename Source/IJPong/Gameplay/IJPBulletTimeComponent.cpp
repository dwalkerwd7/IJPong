// It's Just Pong

#include "Gameplay/IJPBulletTimeComponent.h"
#include "Audio/IJPToneSet.h"
#include "Audio/IJPToneSynthComponent.h"
#include "Era/IJPEra.h"
#include "Era/IJPEraSubsystem.h"
#include "Gameplay/IJPArena.h"
#include "Gameplay/IJPBall.h"
#include "Gameplay/IJPPaddle.h"
#include "Gameplay/IJPPongMath.h"

UIJPBulletTimeComponent::UIJPBulletTimeComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UIJPBulletTimeComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	TimeLeft = FMath::Max(TimeLeft - DeltaTime, 0.f);
	const UIJPEra* Era = UIJPEraSubsystem::GetCurrentEra(this);
	AIJPArena* Arena = GetArena();
	if (!Era || !Era->BulletTime.bEnabled || !Arena)
	{
		TimeLeft = 0.f;
		return;
	}
	const FIJPBulletTime& Settings = Era->BulletTime;

	for (AIJPBall* Ball : Arena->GetBalls())
	{
		if (!Ball->IsInPlay())
		{
			Triggered.Remove(Ball); // served again later: a fresh approach
			continue;
		}
		const int32* Last = Triggered.Find(Ball);
		if (Last && *Last == Ball->GetRallyHits())
		{
			continue; // already had its moment on this approach
		}
		if (TimeLeft <= 0.f && IsNearGoal(*Ball, Settings.Lead))
		{
			Triggered.Add(Ball, Ball->GetRallyHits());
			TimeLeft = Settings.Duration;
			Factor = Settings.TimeScale;
			Arena->GetTones()->PlayTone(Arena->GetToneSet().Warn);
		}
	}
}

void UIJPBulletTimeComponent::Reset()
{
	TimeLeft = 0.f;
	Triggered.Reset();
}

AIJPArena* UIJPBulletTimeComponent::GetArena() const
{
	return Cast<AIJPArena>(GetOwner());
}

bool UIJPBulletTimeComponent::IsNearGoal(const AIJPBall& Ball, float Lead) const
{
	const AIJPArena* Arena = GetArena();
	const FVector2D Position = Ball.GetPlanePosition();
	const FVector2D Velocity = Ball.GetPlaneVelocity();
	if (Ball.IsHeld() || FMath::IsNearlyZero(Velocity.X))
	{
		return false;
	}
	// The paddle guarding the goal it's heading for.
	const EIJPSide Defender = Velocity.X < 0.f ? EIJPSide::Left : EIJPSide::Right;
	const AIJPPaddle* Paddle = Arena->GetPaddle(Defender);
	if (!Paddle)
	{
		return false;
	}
	const float LaneX = Paddle->GetPlanePosition().X;
	const float TimeToLane = (LaneX - Position.X) / Velocity.X;
	if (TimeToLane <= 0.f || TimeToLane > Lead)
	{
		return false; // already past it (too late to matter), or not close yet
	}
	const float BallLimit = Arena->GetHalfExtents().Y - Ball.GetSize() * 0.5f;
	float ArriveY = 0.f;
	if (!FIJPPongMath::PredictInterceptY(Position, Velocity, LaneX, -BallLimit, BallLimit, ArriveY))
	{
		return false;
	}
	float CentreY = 0.f;
	float HalfLength = 0.f;
	Paddle->GetHitSpan(ArriveY, CentreY, HalfLength);
	return FMath::Abs(ArriveY - CentreY) > HalfLength + Ball.GetSize() * 0.5f;
}
