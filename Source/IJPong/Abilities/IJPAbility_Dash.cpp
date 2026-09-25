// It's Just Pong

#include "Abilities/IJPAbility_Dash.h"
#include "Gameplay/IJPPaddle.h"

void UIJPAbility_Dash::Activate()
{
	if (AIJPPaddle* Paddle = GetPaddle())
	{
		Paddle->Dash(Distance, Duration);
	}
}
