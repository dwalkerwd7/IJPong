// It's Just Pong

#include "Gameplay/IJPBossComponent.h"
#include "Abilities/IJPAbilityComponent.h"
#include "Core/IJPGameModeBase.h"
#include "Gameplay/IJPArena.h"
#include "Gameplay/IJPBall.h"
#include "Gameplay/IJPMatchComponent.h"
#include "Gameplay/IJPPaddle.h"
#include "Gameplay/IJPPaddleClass.h"
#include "Gameplay/IJPRival.h"

void UIJPBossComponent::Bind(AIJPArena* InArena, UIJPMatchComponent* InMatch)
{
	Arena = InArena;
	if (Match.IsValid())
	{
		Match->OnHealthChanged.RemoveDynamic(this, &UIJPBossComponent::HandleHealthChanged);
	}
	Match = InMatch;
	if (InMatch)
	{
		InMatch->OnHealthChanged.AddDynamic(this, &UIJPBossComponent::HandleHealthChanged);
	}
}

void UIJPBossComponent::SetBoss(const UIJPRival* InRival)
{
	const bool bWasBoss = IsBossFight();
	Rival = InRival;
	if (IsBossFight() || bWasBoss)
	{
		Restart();
	}
}

bool UIJPBossComponent::IsBossFight() const
{
	return Rival && Rival->IsBoss();
}

void UIJPBossComponent::Restart()
{
	NextPhase = 0;
	AIJPPaddle* Paddle = GetBossPaddle();
	if (!Paddle)
	{
		return;
	}
	Paddle->SetSplitGap(0.f);
	if (!IsBossFight())
	{
		Paddle->SetRunScales(1.f, 1.f);
		return;
	}

	// Its own skill (or its class's) and spell again, and full size.
	UIJPAbilityComponent* Abilities = Paddle->GetAbilities();
	const UIJPPaddleClass* PaddleClass = Paddle->GetPaddleClass();
	Abilities->Equip(EIJPAbilitySlot::ClassSkill, Rival->RivalSkill ? Rival->RivalSkill.Get() : PaddleClass ? PaddleClass->ClassSkill.Get() : nullptr);
	Abilities->Equip(EIJPAbilitySlot::Spell, Rival->Spell);
	UpdateSize(1.f);
}

void UIJPBossComponent::HandleHealthChanged(EIJPSide Side, float Health, float Damage)
{
	const AIJPPaddle* Paddle = GetBossPaddle();
	if (!IsBossFight() || !Paddle || Side != Paddle->GetSide() || !Match.IsValid())
	{
		return;
	}
	const float Max = Match->GetMaxHealth(Side);
	const float Fraction = Max > 0.f ? Health / Max : 0.f;
	UpdateSize(Fraction);

	// Past a threshold (or several at once): into the next stage. Not at 0: that's the end.
	while (Health > 0.f && Rival->Phases.IsValidIndex(NextPhase) && Fraction <= Rival->Phases[NextPhase].AtHealth)
	{
		EnterPhase(NextPhase++);
	}
}

void UIJPBossComponent::EnterPhase(int32 Index)
{
	const FIJPBossPhase& Phase = Rival->Phases[Index];
	AIJPPaddle* Paddle = GetBossPaddle();

	// A beat to see it change.
	if (Phase.Pause > 0.f && Arena.IsValid())
	{
		for (AIJPBall* Ball : Arena->GetBalls())
		{
			if (Ball->IsInPlay())
			{
				Ball->Freeze(Phase.Pause);
			}
		}
	}
	if (AIJPGameModeBase* GameMode = Cast<AIJPGameModeBase>(GetOwner()); GameMode && Phase.Line)
	{
		GameMode->PlayConversation(Phase.Line);
	}

	UIJPAbilityComponent* Abilities = Paddle->GetAbilities();
	if (Phase.Skill)
	{
		Abilities->Equip(EIJPAbilitySlot::ClassSkill, Phase.Skill);
	}
	if (Phase.Spell)
	{
		Abilities->Equip(EIJPAbilitySlot::Spell, Phase.Spell);
		Abilities->AddCharge(1000); // ready at once: the new form shows itself
	}
	if (Phase.SplitGap > 0.f)
	{
		Paddle->SetSplitGap(Phase.SplitGap);
	}
}

void UIJPBossComponent::UpdateSize(float HealthFraction) const
{
	if (AIJPPaddle* Paddle = GetBossPaddle())
	{
		// Huge at full health, its class's size at none. Re-split so the halves follow the new size.
		const float Gap = Paddle->GetSplitGap();
		Paddle->SetRunScales(FMath::Lerp(1.f, Rival->BossLength, FMath::Clamp(HealthFraction, 0.f, 1.f)), 1.f);
		if (Gap > 0.f)
		{
			Paddle->SetSplitGap(Gap);
		}
	}
}

AIJPPaddle* UIJPBossComponent::GetBossPaddle() const
{
	return Arena.IsValid() ? Arena->GetPaddle(IJP::Opposite(AIJPGameModeBase::PlayerSide)) : nullptr;
}
