// It's Just Pong

#include "Abilities/IJPAbility_Smash.h"
#include "Gameplay/IJPBall.h"

void UIJPAbility_Smash::OnBallHit(AIJPBall& Ball)
{
	if (bArmed)
	{
		bArmed = false;
		Ball.Boost(SpeedMultiplier);
	}
}
