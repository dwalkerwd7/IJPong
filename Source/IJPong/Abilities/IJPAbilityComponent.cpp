// It's Just Pong

#include "Abilities/IJPAbilityComponent.h"
#include "AIController.h"
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
	CooldownTotals.SetNumZeroed(NumSlots);
	WindUps.SetNumZeroed(NumSlots);
	Locks.SetNumZeroed(NumSlots);
	Charges.SetNumZeroed(NumSlots);
	CooldownScales.Init(1.f, NumSlots);
}

void UIJPAbilityComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	for (int32 i = 0; i < NumSlots; ++i)
	{
		const bool bWasCooling = Cooldowns[i] > 0.f;
		Cooldowns[i] = FMath::Max(Cooldowns[i] - DeltaTime, 0.f);
		if (bWasCooling && Cooldowns[i] <= 0.f && IsCharged(i))
		{
			SignalReady(i);
		}
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
	Charges[Index] = 0;
	ShowCharge();
	if (Slot == EIJPAbilitySlot::Item)
	{
		bItemSpent = false;
	}
	WindUps[Index] = 0.f;
}

bool UIJPAbilityComponent::TryActivate(EIJPAbilitySlot Slot)
{
	// A second press on an armed skill takes it back, cooldown and all.
	if (UIJPAbility* Armed = GetAbility(Slot); Armed && Armed->CanCancel() && !IsWindingUp(Slot))
	{
		Armed->Deactivate();
		Cooldowns[static_cast<int32>(Slot)] = 0.f;
		PlayArmChirp(true);
		return false;
	}

	if (!IsReady(Slot))
	{
		return false;
	}

	// The cooldown runs from the button, wind-up included.
	const int32 Index = static_cast<int32>(Slot);
	UIJPAbility* Ability = Abilities[Index];
	Cooldowns[Index] = Ability->StartsCooldown() ? Ability->Cooldown * CooldownScales[Index] : 0.f;
	CooldownTotals[Index] = Cooldowns[Index];
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
	if (Ability->ChargeCost > 0)
	{
		Charges[Index] = 0; // spent: charge up again
		ShowCharge();
	}
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

float UIJPAbilityComponent::GetCooldownFraction(EIJPAbilitySlot Slot) const
{
	const int32 Index = static_cast<int32>(Slot);
	return CooldownTotals[Index] > 0.f ? FMath::Clamp(Cooldowns[Index] / CooldownTotals[Index], 0.f, 1.f) : 0.f;
}

bool UIJPAbilityComponent::IsReady(EIJPAbilitySlot Slot) const
{
	const UIJPAbility* Ability = GetAbility(Slot);
	const bool bSpent = Slot == EIJPAbilitySlot::Item && bItemSpent;
	const bool bCharged = !Ability || Ability->ChargeCost <= 0 || GetCharge(Slot) >= Ability->ChargeCost;
	return Ability && !bSpent && bCharged && GetCooldownRemaining(Slot) <= 0.f && !IsWindingUp(Slot) && !IsLocked(Slot) && Ability->CanActivate();
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

void UIJPAbilityComponent::PlayArmChirp(bool bFalling)
{
	const AIJPPaddle* Paddle = GetPaddle();
	AIJPArena* Arena = Paddle ? Paddle->GetArena() : nullptr;
	if (!Arena)
	{
		return;
	}

	const UIJPToneSet& ToneSet = Arena->GetToneSet();
	FIJPTone First = ToneSet.Arm;
	FIJPTone Second = ToneSet.Arm;
	(bFalling ? First : Second).Frequency *= ToneSet.ArmRise;
	Arena->GetTones()->PlayTone(First);
	TWeakObjectPtr<AIJPArena> WeakArena = Arena;
	GetWorld()->GetTimerManager().SetTimer(ChirpTimer, [WeakArena, Second]
	{
		if (WeakArena.IsValid())
		{
			WeakArena->GetTones()->PlayTone(Second);
		}
	}, FMath::Max(ToneSet.Arm.Duration, UE_KINDA_SMALL_NUMBER), false);
}

void UIJPAbilityComponent::AddCharge(int32 Amount)
{
	for (int32 i = 0; i < NumSlots; ++i)
	{
		if (Abilities[i] && Abilities[i]->ChargeCost > 0)
		{
			const bool bWasCharged = IsCharged(i);
			Charges[i] = FMath::Min(Charges[i] + Amount, Abilities[i]->ChargeCost);
			if (!bWasCharged && IsCharged(i) && Cooldowns[i] <= 0.f)
			{
				SignalReady(i);
			}
		}
	}
	ShowCharge();
}

void UIJPAbilityComponent::ShowCharge() const
{
	const AIJPPaddle* Paddle = GetPaddle();
	AIJPArena* Arena = Paddle ? Paddle->GetArena() : nullptr;
	if (!Arena)
	{
		return;
	}
	const int32 Index = static_cast<int32>(EIJPAbilitySlot::Spell);
	const UIJPAbility* Spell = Abilities[Index];
	Arena->SetChargePips(Paddle->GetSide(), Charges[Index], Spell ? Spell->ChargeCost : 0);
}

void UIJPAbilityComponent::HandleBallHit(AIJPBall& Ball)
{
	// Every return charges the spells.
	AddCharge(1);
	for (UIJPAbility* Ability : Abilities)
	{
		if (Ability)
		{
			Ability->OnBallHit(Ball);
		}
	}
}

bool UIJPAbilityComponent::IsCharged(int32 Index) const
{
	const UIJPAbility* Ability = Abilities[Index];
	return !Ability || Ability->ChargeCost <= 0 || Charges[Index] >= Ability->ChargeCost;
}

void UIJPAbilityComponent::SignalReady(int32 Index)
{
	// Only the player needs telling; an AI paddle flashing would just be noise.
	AIJPPaddle* Paddle = GetPaddle();
	AIJPArena* Arena = Paddle ? Paddle->GetArena() : nullptr;
	if (!Abilities[Index] || !Arena || Paddle->GetController<AAIController>())
	{
		return;
	}
	++ReadySignals;
	Paddle->FlashReady();
	const UIJPToneSet& ToneSet = Arena->GetToneSet();
	FIJPTone Tone = ToneSet.Ready;
	if (ToneSet.ReadySlotPitch.IsValidIndex(Index))
	{
		Tone.Frequency *= ToneSet.ReadySlotPitch[Index];
	}
	Arena->GetTones()->PlayTone(Tone);
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
