// It's Just Pong

#include "Abilities/IJPAbility_SlowMo.h"
#include "Gameplay/IJPArena.h"
#include "Gameplay/IJPBall.h"
#include "Gameplay/IJPPaddle.h"

void UIJPAbility_SlowMo::Activate()
{
	TimeLeft = Duration;
	SetBallsTimeScale(TimeScale);
}

void UIJPAbility_SlowMo::TickAbility(float DeltaSeconds)
{
	if (TimeLeft > 0.f)
	{
		// Real time, not the balls' slowed time.
		TimeLeft -= DeltaSeconds;
		if (TimeLeft <= 0.f)
		{
			Deactivate();
		}
	}
}

void UIJPAbility_SlowMo::Deactivate()
{
	TimeLeft = 0.f;
	SetBallsTimeScale(1.f);
}

void UIJPAbility_SlowMo::SetBallsTimeScale(float Scale) const
{
	const AIJPPaddle* Paddle = GetPaddle();
	const AIJPArena* Arena = Paddle ? Paddle->GetArena() : nullptr;
	if (!Arena)
	{
		return;
	}
	for (AIJPBall* Ball : Arena->GetBalls())
	{
		Ball->SetTimeScale(Scale);
	}
}
