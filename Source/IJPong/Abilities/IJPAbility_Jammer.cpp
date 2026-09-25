// It's Just Pong

#include "Abilities/IJPAbility_Jammer.h"
#include "Abilities/IJPAbilityComponent.h"
#include "Gameplay/IJPArena.h"
#include "Gameplay/IJPBall.h"
#include "Gameplay/IJPPaddle.h"
#include "Presentation/IJPCRTComponent.h"

void UIJPAbility_Jammer::Activate()
{
	AIJPPaddle* Target = GetTarget();
	if (!Target)
	{
		return;
	}
	Target->GetAbilities()->LockSlot(EIJPAbilitySlot::ClassSkill, Duration);
	if (UIJPCRTComponent* CRT = Target->GetArena()->GetCRT())
	{
		CRT->Jam(StaticStrength, Duration);
	}
}

bool UIJPAbility_Jammer::WantsAIUse() const
{
	const AIJPPaddle* Target = GetTarget();
	if (!Target || !Target->GetAbilities()->IsReady(EIJPAbilitySlot::ClassSkill))
	{
		return false;
	}
	const float TowardThem = IJP::SideSign(Target->GetSide());
	for (const AIJPBall* Ball : Target->GetArena()->GetBalls())
	{
		if (Ball->IsInPlay() && Ball->GetPlaneVelocity().X * TowardThem > 0.f)
		{
			return true;
		}
	}
	return false;
}

AIJPPaddle* UIJPAbility_Jammer::GetTarget() const
{
	const AIJPPaddle* Paddle = GetPaddle();
	AIJPArena* Arena = Paddle ? Paddle->GetArena() : nullptr;
	return Arena ? Arena->GetPaddle(IJP::Opposite(Paddle->GetSide())) : nullptr;
}
