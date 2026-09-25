// It's Just Pong

#include "Abilities/IJPAbilityComponent.h"
#include "Gameplay/IJPPaddle.h"

namespace
{
	constexpr int32 NumSlots = static_cast<int32>(EIJPAbilitySlot::Count);
}

UIJPAbilityComponent::UIJPAbilityComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	Abilities.SetNum(NumSlots);
	Cooldowns.SetNumZeroed(NumSlots);
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
	Cooldowns[Index] = Ability->Cooldown;
	OnActivated.Broadcast(Slot, Ability);
	return true;
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
