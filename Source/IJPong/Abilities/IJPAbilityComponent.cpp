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
	WindUps.SetNumZeroed(NumSlots);
	Locks.SetNumZeroed(NumSlots);
	CooldownScales.Init(1.f, NumSlots);
}

void UIJPAbilityComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	for (int32 i = 0; i < NumSlots; ++i)
	{
		Cooldowns[i] = FMath::Max(Cooldowns[i] - DeltaTime, 0.f);
		Locks[i] = FMath::Max(Locks[i] - DeltaTime, 0.f);
		if (WindUps[i] > 0.f)
		{
			WindUps[i] -= DeltaTime;
			if (WindUps[i] <= 0.f)
			{
				WindUps[i] = 0.f;
				Fire(i);
			}
		}
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
		Copy->Init(this, Definition);
	}
	Abilities[Index] = Copy;
	Cooldowns[Index] = 0.f;
	if (Slot == EIJPAbilitySlot::Item)
	{
		bItemSpent = false;
	}
	WindUps[Index] = 0.f;
}

bool UIJPAbilityComponent::TryActivate(EIJPAbilitySlot Slot)
{
	if (!IsReady(Slot))
	{
		return false;
	}

	// The cooldown runs from the button, wind-up included.
	const int32 Index = static_cast<int32>(Slot);
	UIJPAbility* Ability = Abilities[Index];
	Cooldowns[Index] = Ability->Cooldown * CooldownScales[Index];
	if (Ability->Telegraph > 0.f)
	{
		WindUps[Index] = Ability->Telegraph;
		PlayWarning();
	}
	else
	{
		Fire(Index);
	}
	return true;
}

void UIJPAbilityComponent::Fire(int32 Index)
{
	UIJPAbility* Ability = Abilities[Index];
	if (!Ability)
	{
		return;
	}
	Ability->Activate();
	if (Index == static_cast<int32>(EIJPAbilitySlot::Item))
	{
		bItemSpent = true; // one use
	}
	if (Ability->IsArmed())
	{
		PlayArmChirp();
	}
	OnActivated.Broadcast(static_cast<EIJPAbilitySlot>(Index), Ability);
}

bool UIJPAbilityComponent::HasItem() const
{
	return GetAbility(EIJPAbilitySlot::Item) && !bItemSpent;
}

bool UIJPAbilityComponent::IsWindingUp(EIJPAbilitySlot Slot) const
{
	return WindUps[static_cast<int32>(Slot)] > 0.f;
}

bool UIJPAbilityComponent::ShouldShowCue() const
{
	return IsArmed() || WindUps.ContainsByPredicate([](float Time) { return Time > 0.f; });
}

void UIJPAbilityComponent::PlayWarning()
{
	const AIJPPaddle* Paddle = GetPaddle();
	if (AIJPArena* Arena = Paddle ? Paddle->GetArena() : nullptr)
	{
		Arena->GetTones()->PlayTone(Arena->GetToneSet().Warn);
	}
}

void UIJPAbilityComponent::Release(EIJPAbilitySlot Slot)
{
	if (UIJPAbility* Ability = GetAbility(Slot))
	{
		Ability->OnButtonReleased();
	}
}

void UIJPAbilityComponent::LockSlot(EIJPAbilitySlot Slot, float Seconds)
{
	float& Lock = Locks[static_cast<int32>(Slot)];
	Lock = FMath::Max(Lock, Seconds);
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
	const bool bSpent = Slot == EIJPAbilitySlot::Item && bItemSpent;
	return Ability && !bSpent && GetCooldownRemaining(Slot) <= 0.f && !IsWindingUp(Slot) && !IsLocked(Slot) && Ability->CanActivate();
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
	for (float& WindUp : WindUps)
	{
		WindUp = 0.f;
	}
	for (UIJPAbility* Ability : Abilities)
	{
		if (Ability)
		{
			Ability->Deactivate();
		}
	}
	Super::EndPlay(EndPlayReason);
}
