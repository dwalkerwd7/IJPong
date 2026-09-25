// It's Just Pong

#include "Abilities/IJPAbility_Snare.h"
#include "Gameplay/IJPArena.h"
#include "Gameplay/IJPBall.h"
#include "Gameplay/IJPPaddle.h"

void UIJPAbility_Snare::Activate()
{
	if (AIJPPaddle* Target = GetTarget())
	{
		Snared = Target;
		TimeLeft = Duration;
		Target->SetSpeedScale(SpeedMultiplier);
	}
}

bool UIJPAbility_Snare::WantsAIUse() const
{
	// A ball on its way to the other side: they're about to have to move for it.
	const AIJPPaddle* Paddle = GetPaddle();
	const AIJPPaddle* Target = GetTarget();
	if (!Paddle || !Target)
	{
		return false;
	}
	const float TowardTarget = IJP::SideSign(Target->GetSide());
	for (const AIJPBall* Ball : Paddle->GetArena()->GetBalls())
	{
		if (Ball->IsInPlay() && Ball->GetPlaneVelocity().X * TowardTarget > 0.f)
		{
			return true;
		}
	}
	return false;
}

void UIJPAbility_Snare::TickAbility(float DeltaSeconds)
{
	if (TimeLeft > 0.f)
	{
		TimeLeft -= DeltaSeconds;
		if (TimeLeft <= 0.f)
		{
			Deactivate();
		}
	}
}

void UIJPAbility_Snare::Deactivate()
{
	TimeLeft = 0.f;
	if (Snared.IsValid())
	{
		Snared->SetSpeedScale(1.f);
	}
	Snared.Reset();
}

AIJPPaddle* UIJPAbility_Snare::GetTarget() const
{
	const AIJPPaddle* Paddle = GetPaddle();
	AIJPArena* Arena = Paddle ? Paddle->GetArena() : nullptr;
	return Arena ? Arena->GetPaddle(IJP::Opposite(Paddle->GetSide())) : nullptr;
}
