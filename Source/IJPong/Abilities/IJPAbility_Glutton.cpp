// It's Just Pong

#include "Abilities/IJPAbility_Glutton.h"
#include "Gameplay/IJPArena.h"
#include "Gameplay/IJPBall.h"
#include "Gameplay/IJPPaddle.h"

void UIJPAbility_Glutton::Activate()
{
	TimeLeft = Duration;
}

bool UIJPAbility_Glutton::WantsAIUse() const
{
	const AIJPPaddle* Paddle = GetPaddle();
	const AIJPArena* Arena = Paddle ? Paddle->GetArena() : nullptr;
	if (!Arena || Arena->GetNumBallsInPlay() < 2)
	{
		return false;
	}
	const float TowardMe = IJP::SideSign(Paddle->GetSide());
	for (const AIJPBall* Ball : Arena->GetBalls())
	{
		if (Ball->IsInPlay() && Ball->GetPlaneVelocity().X * TowardMe > 0.f)
		{
			return true;
		}
	}
	return false;
}

void UIJPAbility_Glutton::TickAbility(float DeltaSeconds)
{
	if (TimeLeft <= 0.f)
	{
		return;
	}
	if (AIJPBall* Meal = FindMeal())
	{
		Meal->ResetBall();
		++Swallowed;
		TimeLeft = 0.f;
		return;
	}
	TimeLeft -= DeltaSeconds;
}

AIJPBall* UIJPAbility_Glutton::FindMeal() const
{
	const AIJPPaddle* Paddle = GetPaddle();
	const AIJPArena* Arena = Paddle ? Paddle->GetArena() : nullptr;
	if (!Arena || Arena->GetNumBallsInPlay() < 2)
	{
		return nullptr;
	}

	// The one closest to this goal, among those on this half heading here.
	const float TowardMe = IJP::SideSign(Paddle->GetSide());
	AIJPBall* Closest = nullptr;
	for (AIJPBall* Ball : Arena->GetBalls())
	{
		const bool bOnMyHalf = Ball->GetPlanePosition().X * TowardMe > 0.f;
		const bool bComing = Ball->GetPlaneVelocity().X * TowardMe > 0.f;
		if (Ball->IsInPlay() && bOnMyHalf && bComing
			&& (!Closest || Ball->GetPlanePosition().X * TowardMe > Closest->GetPlanePosition().X * TowardMe))
		{
			Closest = Ball;
		}
	}
	return Closest;
}
