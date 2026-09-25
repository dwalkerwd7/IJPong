// It's Just Pong

#include "Abilities/IJPAbility_Smash.h"
#include "Abilities/IJPAbility_Curve.h"
#include "Abilities/IJPAbility_Split.h"
#include "Gameplay/IJPBall.h"
#include "Gameplay/IJPPaddle.h"

namespace
{
	const FName PowerUpgrade(TEXT("Power"));
	const FName CurveUpgrade(TEXT("Curve"));
	const FName ExtraHitsUpgrade(TEXT("ExtraHits"));
	const FName SplitUpgrade(TEXT("Split"));
}

void UIJPAbility_Smash::Activate()
{
	HitsLeft = 1 + FMath::Max(FMath::RoundToInt(GetUpgrade(ExtraHitsUpgrade)), 0);
}

void UIJPAbility_Smash::OnBallHit(AIJPBall& Ball)
{
	if (HitsLeft <= 0)
	{
		return;
	}
	--HitsLeft;

	// Split first (it sets the ball's heading, which would drop a boost), then smash, then spin.
	AIJPPaddle* Paddle = GetPaddle();
	if (Paddle && Paddle->GetArena() && GetUpgrade(SplitUpgrade) > 0.f)
	{
		UIJPAbility_Split::FanOut(Ball, *Paddle->GetArena(), 30.f);
	}
	Ball.Boost(SpeedMultiplier + GetUpgrade(PowerUpgrade));
	if (Paddle && GetUpgrade(CurveUpgrade) > 0.f)
	{
		UIJPAbility_Curve::CurveReturn(Ball, *Paddle, 70.f * GetUpgrade(CurveUpgrade), 0.9f);
	}
}
