// It's Just Pong

#include "Abilities/IJPAbility_Grow.h"
#include "Gameplay/IJPPaddle.h"

void UIJPAbility_Grow::Activate()
{
	if (AIJPPaddle* Paddle = GetPaddle())
	{
		TimeLeft = Duration;
		Paddle->SetLengthScale(LengthMultiplier);
	}
}

void UIJPAbility_Grow::TickAbility(float DeltaSeconds)
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

void UIJPAbility_Grow::Deactivate()
{
	TimeLeft = 0.f;
	if (AIJPPaddle* Paddle = GetPaddle())
	{
		Paddle->SetLengthScale(1.f);
	}
}
