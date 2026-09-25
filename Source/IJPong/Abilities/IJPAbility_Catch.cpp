// It's Just Pong

#include "Abilities/IJPAbility_Catch.h"
#include "Gameplay/IJPBall.h"
#include "Gameplay/IJPPaddle.h"

void UIJPAbility_Catch::OnBallHit(AIJPBall& Ball)
{
	AIJPPaddle* Paddle = GetPaddle();
	if (!bArmed || Held.IsValid() || !Paddle)
	{
		return;
	}
	bArmed = false;

	// The return has already bounced: keep its speed, and start the aim at its angle.
	const FVector2D Velocity = Ball.GetPlaneVelocity();
	FireSpeed = Velocity.Size();
	Paddle->SetAimAngle(FMath::RadiansToDegrees(FMath::Atan2(Velocity.Y, FMath::Abs(Velocity.X))));
	Held = &Ball;
	HoldLeft = HoldTime;
	Ball.Hold(Paddle);
}

void UIJPAbility_Catch::OnButtonReleased()
{
	if (Held.IsValid())
	{
		Fire();
	}
}

void UIJPAbility_Catch::TickAbility(float DeltaSeconds)
{
	AIJPPaddle* Paddle = GetPaddle();
	if (!Held.IsValid() || !Paddle)
	{
		return;
	}
	if (!Held->IsHeld())
	{
		// Taken off us some other way (a new serve, the match ending).
		Held.Reset();
		Paddle->HideAim();
		return;
	}

	Paddle->ShowAim(Held->GetPlanePosition(), GetFireAngle(), AimLineLength);
	HoldLeft -= DeltaSeconds;
	if (HoldLeft <= 0.f)
	{
		Fire();
	}
}

void UIJPAbility_Catch::Deactivate()
{
	bArmed = false;
	if (Held.IsValid())
	{
		Fire();
	}
}

float UIJPAbility_Catch::GetFireAngle() const
{
	const AIJPPaddle* Paddle = GetPaddle();
	return Paddle ? FMath::Clamp(Paddle->GetAimAngle(), -AimLimitDeg, AimLimitDeg) : 0.f;
}

void UIJPAbility_Catch::Fire()
{
	AIJPPaddle* Paddle = GetPaddle();
	AIJPBall* Ball = Held.Get();
	Held.Reset();
	if (!Paddle || !Ball)
	{
		return;
	}
	Paddle->HideAim();
	Ball->Release(Paddle->AimAngleToDirection(GetFireAngle()) * FireSpeed);
}
