// It's Just Pong

#include "Abilities/IJPAbility_Scorcher.h"
#include "Abilities/IJPAbility_Catch.h"
#include "Abilities/IJPAbilityComponent.h"
#include "Gameplay/IJPArena.h"
#include "Gameplay/IJPBall.h"
#include "Gameplay/IJPPaddle.h"

bool UIJPAbility_Scorcher::WantsAIUse() const
{
	const AIJPPaddle* Paddle = GetPaddle();
	AIJPArena* Arena = Paddle ? Paddle->GetArena() : nullptr;
	const AIJPPaddle* Other = Arena ? Arena->GetPaddle(IJP::Opposite(Paddle->GetSide())) : nullptr;
	if (!Other || !Cast<UIJPAbility_Catch>(Other->GetAbilities()->GetAbility(EIJPAbilitySlot::ClassSkill)))
	{
		return false;
	}
	// Heat up for a ball on its way here, so the return goes back hot.
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

void UIJPAbility_Scorcher::OnBallHit(AIJPBall& Ball)
{
	if (TimeLeft > 0.f)
	{
		Ball.SetHeat(Heat);
	}
}
