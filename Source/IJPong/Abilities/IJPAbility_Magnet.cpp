// It's Just Pong

#include "Abilities/IJPAbility_Magnet.h"
#include "Gameplay/IJPArena.h"
#include "Gameplay/IJPBall.h"
#include "Gameplay/IJPPaddle.h"

void UIJPAbility_Magnet::Activate()
{
	TimeLeft = Duration;
	Pulled.Reset();
	Pull();
}

bool UIJPAbility_Magnet::WantsAIUse() const
{
	const AIJPPaddle* Paddle = GetPaddle();
	if (!Paddle || !Paddle->GetArena())
	{
		return false;
	}
	for (const AIJPBall* Ball : Paddle->GetArena()->GetBalls())
	{
		if (IsIncoming(*Ball) && (Ball->IsCurving() || Ball->IsBoosted()))
		{
			return true;
		}
	}
	return false;
}

void UIJPAbility_Magnet::TickAbility(float DeltaSeconds)
{
	if (TimeLeft > 0.f)
	{
		Pull();
		TimeLeft -= DeltaSeconds;
		if (TimeLeft <= 0.f)
		{
			Deactivate();
		}
	}
}

void UIJPAbility_Magnet::Deactivate()
{
	TimeLeft = 0.f;
	Pulled.Reset();
}

void UIJPAbility_Magnet::Pull()
{
	const AIJPPaddle* Paddle = GetPaddle();
	if (!Paddle || !Paddle->GetArena())
	{
		return;
	}
	for (AIJPBall* Ball : Paddle->GetArena()->GetBalls())
	{
		if (IsIncoming(*Ball) && !Pulled.Contains(Ball))
		{
			Ball->Dampen(CurveScale, BoostScale);
			Pulled.Add(Ball);
		}
	}
}

bool UIJPAbility_Magnet::IsIncoming(const AIJPBall& Ball) const
{
	const AIJPPaddle* Paddle = GetPaddle();
	return Paddle && Ball.IsInPlay() && Ball.GetPlaneVelocity().X * IJP::SideSign(Paddle->GetSide()) > 0.f;
}
