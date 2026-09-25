// It's Just Pong

#include "Abilities/IJPAbility_Freeze.h"
#include "Gameplay/IJPArena.h"
#include "Gameplay/IJPBall.h"
#include "Gameplay/IJPPaddle.h"

void UIJPAbility_Freeze::Activate()
{
	const AIJPPaddle* Paddle = GetPaddle();
	const AIJPArena* Arena = Paddle ? Paddle->GetArena() : nullptr;
	if (!Arena)
	{
		return;
	}
	for (AIJPBall* Ball : Arena->GetBalls())
	{
		Ball->Freeze(Duration);
	}
}
