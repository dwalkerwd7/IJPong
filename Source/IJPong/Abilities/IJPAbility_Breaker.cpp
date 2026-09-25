// It's Just Pong

#include "Abilities/IJPAbility_Breaker.h"
#include "Abilities/IJPAbility_Barrier.h"
#include "Abilities/IJPAbilityComponent.h"
#include "Gameplay/IJPArena.h"
#include "Gameplay/IJPBall.h"
#include "Gameplay/IJPPaddle.h"

bool UIJPAbility_Breaker::WantsAIUse() const
{
	const AIJPPaddle* Paddle = GetPaddle();
	const AIJPArena* Arena = Paddle ? Paddle->GetArena() : nullptr;
	const AIJPPaddle* Other = Arena ? Arena->GetPaddle(IJP::Opposite(Paddle->GetSide())) : nullptr;
	if (!Other)
	{
		return false;
	}

	// Only worth it against a barrier, up now or one they can raise.
	const bool bBarrier = Arena->IsBarrierUp(Other->GetSide())
		|| Cast<UIJPAbility_Barrier>(Other->GetAbilities()->GetAbility(EIJPAbilitySlot::ClassSkill)) != nullptr;
	if (!bBarrier)
	{
		return false;
	}

	// Arm for a ball on its way here, so the next return carries it.
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

void UIJPAbility_Breaker::OnBallHit(AIJPBall& Ball)
{
	if (bArmed)
	{
		bArmed = false;
		Ball.SetPiercing(true);
	}
}
