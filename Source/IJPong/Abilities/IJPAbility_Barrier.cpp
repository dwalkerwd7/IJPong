// It's Just Pong

#include "Abilities/IJPAbility_Barrier.h"
#include "Gameplay/IJPArena.h"
#include "Gameplay/IJPPaddle.h"

void UIJPAbility_Barrier::Activate()
{
	TimeLeft = Duration;
	SetUp(true);
}

void UIJPAbility_Barrier::TickAbility(float DeltaSeconds)
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

void UIJPAbility_Barrier::Deactivate()
{
	TimeLeft = 0.f;
	SetUp(false);
}

void UIJPAbility_Barrier::SetUp(bool bUp) const
{
	const AIJPPaddle* Paddle = GetPaddle();
	if (AIJPArena* Arena = Paddle ? Paddle->GetArena() : nullptr)
	{
		Arena->SetBarrierUp(Paddle->GetSide(), bUp);
	}
}
