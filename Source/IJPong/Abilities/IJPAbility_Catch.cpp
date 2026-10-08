// It's Just Pong

#include "Abilities/IJPAbility_Catch.h"
#include "Abilities/IJPAbility_Curve.h"
#include "Gameplay/IJPBall.h"
#include "Gameplay/IJPPaddle.h"

namespace IJPCatchUpgrades
{
	const FName HoldTimeUpgrade(TEXT("HoldTime"));
	const FName AimLimitUpgrade(TEXT("AimLimit"));
	const FName PowerUpgrade(TEXT("Power"));
	const FName ExtraCatchesUpgrade(TEXT("ExtraCatches"));
	const FName SpinUpgrade(TEXT("Spin"));
}

void UIJPAbility_Catch::Activate()
{
	bArmed = true;
	CatchesLeft = 1 + FMath::Max(FMath::RoundToInt(GetUpgrade(IJPCatchUpgrades::ExtraCatchesUpgrade)), 0);
}

void UIJPAbility_Catch::OnBallHit(AIJPBall& Ball)
{
	AIJPPaddle* Paddle = GetPaddle();
	if (!bArmed || Held.IsValid() || !Paddle)
	{
		return;
	}
	bArmed = false;
	--CatchesLeft;

	// The return has already bounced: keep its speed, and start the aim at its angle.
	const FVector2D Velocity = Ball.GetPlaneVelocity();
	FireSpeed = Velocity.Size();
	Paddle->SetAimAngle(FMath::RadiansToDegrees(FMath::Atan2(Velocity.Y, FMath::Abs(Velocity.X))));
	Held = &Ball;
	// A hot ball (a rival's Scorcher) can't be held as long.
	HoldLeft = (HoldTime + GetUpgrade(IJPCatchUpgrades::HoldTimeUpgrade)) * (1.f - Ball.GetArrivalHeat());
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
	CatchesLeft = 0;
	if (Held.IsValid())
	{
		Fire();
	}
}

float UIJPAbility_Catch::GetFireAngle() const
{
	const AIJPPaddle* Paddle = GetPaddle();
	const float Limit = FMath::Min(AimLimitDeg + GetUpgrade(IJPCatchUpgrades::AimLimitUpgrade), 85.f);
	return Paddle ? FMath::Clamp(Paddle->GetAimAngle(), -Limit, Limit) : 0.f;
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
	if (GetUpgrade(IJPCatchUpgrades::PowerUpgrade) > 0.f)
	{
		Ball->Boost(PowerBoost);
	}
	if (GetUpgrade(IJPCatchUpgrades::SpinUpgrade) > 0.f)
	{
		UIJPAbility_Curve::CurveReturn(*Ball, *Paddle, SpinRate, SpinTime);
	}
	// Double Catch: ready for the next return straight away.
	bArmed = CatchesLeft > 0;
}
