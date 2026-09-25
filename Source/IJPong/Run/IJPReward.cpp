// It's Just Pong

#include "Run/IJPReward.h"
#include "Run/IJPRunSubsystem.h"

void UIJPReward_Modifier::Grant(UIJPRunSubsystem& Run) const
{
	FIJPRunLoadout& Loadout = Run.EditLoadout();
	switch (Stat)
	{
	case EIJPRunStat::PaddleLength:       Loadout.PaddleLengthBonus += Amount; break;
	case EIJPRunStat::PaddleSpeed:        Loadout.PaddleSpeedBonus += Amount; break;
	case EIJPRunStat::ClassSkillCooldown: Loadout.ClassSkillCooldownCut += Amount; break;
	case EIJPRunStat::MaxHealth:          Run.AddMaxHealth(Amount); break;
	}
}

bool UIJPReward_Ability::CanOffer(const UIJPRunSubsystem& Run) const
{
	return Ability && Run.GetLoadout().RunAbility != Ability;
}

void UIJPReward_Ability::Grant(UIJPRunSubsystem& Run) const
{
	Run.EditLoadout().RunAbility = Ability;
}

void UIJPReward_Ball::Grant(UIJPRunSubsystem& Run) const
{
	if (BallType)
	{
		Run.EditLoadout().ExtraServedBalls.Add(BallType);
	}
}
