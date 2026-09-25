// It's Just Pong

#include "Abilities/IJPItem_Patch.h"
#include "Core/IJPGameModeBase.h"
#include "Engine/World.h"
#include "Gameplay/IJPMatchComponent.h"
#include "Gameplay/IJPPaddle.h"

void UIJPItem_Patch::Activate()
{
	// Health lives in the match; a run follows it through OnHealthChanged.
	const AIJPPaddle* Paddle = GetPaddle();
	const AIJPGameModeBase* GameMode = Paddle ? Paddle->GetWorld()->GetAuthGameMode<AIJPGameModeBase>() : nullptr;
	if (UIJPMatchComponent* Match = GameMode ? GameMode->GetMatch() : nullptr)
	{
		Match->Heal(Paddle->GetSide(), Heal);
	}
}
