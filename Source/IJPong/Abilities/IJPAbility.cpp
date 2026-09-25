// It's Just Pong

#include "Abilities/IJPAbility.h"
#include "Abilities/IJPAbilityComponent.h"

void UIJPAbility::Init(UIJPAbilityComponent* InOwner)
{
	Owner = InOwner;
}

AIJPPaddle* UIJPAbility::GetPaddle() const
{
	return Owner.IsValid() ? Owner->GetPaddle() : nullptr;
}
