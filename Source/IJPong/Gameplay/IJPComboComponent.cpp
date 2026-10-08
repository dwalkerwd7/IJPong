// It's Just Pong

#include "Gameplay/IJPComboComponent.h"
#include "Audio/IJPToneSet.h"
#include "Audio/IJPToneSynthComponent.h"
#include "Era/IJPEra.h"
#include "Era/IJPEraSubsystem.h"
#include "Gameplay/IJPArena.h"
#include "Gameplay/IJPBall.h"
#include "Gameplay/IJPPaddle.h"
#include "Presentation/IJPScreenShakeComponent.h"

void UIJPComboComponent::BeginPlay()
{
	Super::BeginPlay();
	if (AIJPArena* Arena = GetArena())
	{
		Arena->OnBallReturned.AddDynamic(this, &UIJPComboComponent::HandleReturn);
		Arena->OnBallGoal.AddDynamic(this, &UIJPComboComponent::HandleGoal);
	}
}

void UIJPComboComponent::Reset()
{
	for (const EIJPSide Side : { EIJPSide::Left, EIJPSide::Right })
	{
		Combo[Index(Side)] = 0;
		SetSuperReady(Side, false);
	}
}

void UIJPComboComponent::HandleReturn(AIJPBall* Ball, AIJPPaddle* Paddle)
{
	AIJPArena* Arena = GetArena();
	const UIJPEra* Era = UIJPEraSubsystem::GetCurrentEra(this);
	if (!Arena || !Ball || !Paddle || !Era || !Era->Combo.bEnabled)
	{
		return;
	}
	const FIJPCombo& Settings = Era->Combo;
	const EIJPSide Side = Paddle->GetSide();
	if (IsSuperReady(Side))
	{
		// The super shot.
		Ball->Boost(Settings.SuperBoost);
		Ball->SetDamageScale(Settings.SuperDamage);
		SetSuperReady(Side, false);
		Arena->GetScreenShake()->Shake(0.25f, 4.f);
		Arena->GetTones()->PlayTone(Arena->GetToneSet().Pop);
		return;
	}
	if (++Combo[Index(Side)] >= Settings.ReturnsToFill)
	{
		Combo[Index(Side)] = 0;
		SetSuperReady(Side, true);
		Arena->GetTones()->PlayTone(Arena->GetToneSet().Arm);
	}
}

void UIJPComboComponent::HandleGoal(AIJPBall* Ball, EIJPSide DefendingSide)
{
	// A goal breaks the combos; a ready super stays ready.
	Combo[0] = Combo[1] = 0;
}

AIJPArena* UIJPComboComponent::GetArena() const
{
	return Cast<AIJPArena>(GetOwner());
}

void UIJPComboComponent::SetSuperReady(EIJPSide Side, bool bReady)
{
	bSuperReady[Index(Side)] = bReady;
	if (const AIJPArena* Arena = GetArena())
	{
		if (AIJPPaddle* Paddle = Arena->GetPaddle(Side))
		{
			Paddle->SetExtraArmedCue(bReady);
		}
	}
}
