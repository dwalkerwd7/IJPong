// It's Just Pong

#include "Abilities/IJPItem_Shield.h"
#include "Gameplay/IJPArena.h"
#include "Gameplay/IJPPaddle.h"

void UIJPItem_Shield::Activate()
{
	const AIJPPaddle* Paddle = GetPaddle();
	AIJPArena* Arena = Paddle ? Paddle->GetArena() : nullptr;
	if (!Arena)
	{
		return;
	}
	Arena->SetBarrierUp(Paddle->GetSide(), true);
	Arena->OnBarrierHit.AddUniqueDynamic(this, &UIJPItem_Shield::HandleBarrierHit);
	bUp = true;
}

void UIJPItem_Shield::HandleBarrierHit(EIJPSide Side, AIJPBall* Ball)
{
	const AIJPPaddle* Paddle = GetPaddle();
	if (bUp && Paddle && Side == Paddle->GetSide())
	{
		Lower();
	}
}

void UIJPItem_Shield::Lower()
{
	if (!bUp)
	{
		return;
	}
	bUp = false;
	const AIJPPaddle* Paddle = GetPaddle();
	if (AIJPArena* Arena = Paddle ? Paddle->GetArena() : nullptr)
	{
		Arena->SetBarrierUp(Paddle->GetSide(), false);
		Arena->OnBarrierHit.RemoveDynamic(this, &UIJPItem_Shield::HandleBarrierHit);
	}
}
