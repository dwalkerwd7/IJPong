// It's Just Pong

#include "Abilities/IJPAbilityComponent.h"
#include "Audio/IJPToneSet.h"
#include "Audio/IJPToneSynthComponent.h"
#include "Engine/World.h"
#include "Gameplay/IJPArena.h"
#include "Gameplay/IJPPaddle.h"
#include "TimerManager.h"

namespace
{
	constexpr int32 NumSlots = static_cast<int32>(EIJPAbilitySlot::Count);
}

UIJPAbilityComponent::UIJPAbilityComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	Abilities.SetNum(NumSlots);
	Cooldowns.SetNumZeroed(NumSlots);
	CooldownScales.Init(1.f, NumSlots);
}

void UIJPAbilityComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	for (int32 i = 0; i < NumSlots; ++i)
	{
		Cooldowns[i] = FMath::Max(Cooldowns[i] - DeltaTime, 0.f);
		if (Abilities[i])
		{
			Abilities[i]->TickAbility(DeltaTime);
		}
	}
}

void UIJPAbilityComponent::Equip(EIJPAbilitySlot Slot, const UIJPAbility* Definition)
{
	const int32 Index = static_cast<int32>(Slot);
	if (Abilities[Index])
	{
		Abilities[Index]->Deactivate();
	}

	// The slot's own copy, so its state is per paddle and the asset stays untouched.
	UIJPAbility* Copy = Definition ? DuplicateObject<UIJPAbility>(Definition, this) : nullptr;
	if (Copy)
	{
		Copy->Init(this);
	}
	Abilities[Index] = Copy;
	Cooldowns[Index] = 0.f;
}

bool UIJPAbilityComponent::TryActivate(EIJPAbilitySlot Slot)
{
	if (!IsReady(Slot))
	{
		return false;
	}

	const int32 Index = static_cast<int32>(Slot);
	UIJPAbility* Ability = Abilities[Index];
	Ability->Activate();
	Cooldowns[Index] = Ability->Cooldown * CooldownScales[Index];
	if (Ability->IsArmed())
	{
		PlayArmChirp();
	}
	OnActivated.Broadcast(Slot, Ability);
	return true;
}

void UIJPAbilityComponent::SetCooldownScale(EIJPAbilitySlot Slot, float Scale)
{
	CooldownScales[static_cast<int32>(Slot)] = FMath::Max(Scale, 0.f);
}

UIJPAbility* UIJPAbilityComponent::GetAbility(EIJPAbilitySlot Slot) const
{
	return Abilities[static_cast<int32>(Slot)];
}

float UIJPAbilityComponent::GetCooldownRemaining(EIJPAbilitySlot Slot) const
{
	return Cooldowns[static_cast<int32>(Slot)];
}

bool UIJPAbilityComponent::IsReady(EIJPAbilitySlot Slot) const
{
	const UIJPAbility* Ability = GetAbility(Slot);
	return Ability && GetCooldownRemaining(Slot) <= 0.f && Ability->CanActivate();
}

bool UIJPAbilityComponent::IsArmed() const
{
	for (const UIJPAbility* Ability : Abilities)
	{
		if (Ability && Ability->IsArmed())
		{
			return true;
		}
	}
	return false;
}

void UIJPAbilityComponent::PlayArmChirp()
{
	const AIJPPaddle* Paddle = GetPaddle();
	AIJPArena* Arena = Paddle ? Paddle->GetArena() : nullptr;
	if (!Arena)
	{
		return;
	}

	const UIJPToneSet& ToneSet = Arena->GetToneSet();
	FIJPTone Second = ToneSet.Arm;
	Second.Frequency *= ToneSet.ArmRise;
	Arena->GetTones()->PlayTone(ToneSet.Arm);
	TWeakObjectPtr<AIJPArena> WeakArena = Arena;
	GetWorld()->GetTimerManager().SetTimer(ChirpTimer, [WeakArena, Second]
	{
		if (WeakArena.IsValid())
		{
			WeakArena->GetTones()->PlayTone(Second);
		}
	}, FMath::Max(ToneSet.Arm.Duration, UE_KINDA_SMALL_NUMBER), false);
}

void UIJPAbilityComponent::HandleBallHit(AIJPBall& Ball)
{
	for (UIJPAbility* Ability : Abilities)
	{
		if (Ability)
		{
			Ability->OnBallHit(Ball);
		}
	}
}

AIJPPaddle* UIJPAbilityComponent::GetPaddle() const
{
	return Cast<AIJPPaddle>(GetOwner());
}

void UIJPAbilityComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	for (UIJPAbility* Ability : Abilities)
	{
		if (Ability)
		{
			Ability->Deactivate();
		}
	}
	Super::EndPlay(EndPlayReason);
}
