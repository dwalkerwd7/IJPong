// It's Just Pong

#include "Abilities/IJPAbility_Dash.h"
#include "Gameplay/IJPBall.h"
#include "Gameplay/IJPPaddle.h"

namespace
{
	const FName DistanceUpgrade(TEXT("Distance"));
	const FName ExtraDashesUpgrade(TEXT("ExtraDashes"));
	const FName SlipstreamUpgrade(TEXT("Slipstream"));
	const FName DashStrikeUpgrade(TEXT("DashStrike"));
}

void UIJPAbility_Dash::Activate()
{
	AIJPPaddle* Paddle = GetPaddle();
	if (!Paddle)
	{
		return;
	}
	Paddle->Dash(Distance * (1.f + GetUpgrade(DistanceUpgrade)), Duration);
	SinceDash = 0.f;
	if (GetUpgrade(SlipstreamUpgrade) > 0.f)
	{
		Paddle->BoostSpeed(SlipstreamSpeed, SlipstreamTime);
	}
}

bool UIJPAbility_Dash::StartsCooldown()
{
	// Double Dash: the first dashes of a cycle are free; the last one starts the cooldown.
	if (++DashesInCycle <= FMath::RoundToInt(GetUpgrade(ExtraDashesUpgrade)))
	{
		return false;
	}
	DashesInCycle = 0;
	return true;
}

void UIJPAbility_Dash::TickAbility(float DeltaSeconds)
{
	SinceDash += DeltaSeconds;
}

void UIJPAbility_Dash::OnBallHit(AIJPBall& Ball)
{
	if (GetUpgrade(DashStrikeUpgrade) > 0.f && SinceDash <= StrikeWindow)
	{
		Ball.Boost(StrikeBoost);
		SinceDash = TNumericLimits<float>::Max(); // one strike per dash
	}
}
