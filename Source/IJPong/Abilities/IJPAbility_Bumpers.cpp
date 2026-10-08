// It's Just Pong

#include "Abilities/IJPAbility_Bumpers.h"
#include "Engine/World.h"
#include "Gameplay/IJPArena.h"
#include "Gameplay/IJPBonusTarget.h"
#include "Gameplay/IJPBonusTargetComponent.h"
#include "Gameplay/IJPPaddle.h"

void UIJPAbility_Bumpers::Activate()
{
	AIJPPaddle* Paddle = GetPaddle();
	AIJPArena* Arena = Paddle ? Paddle->GetArena() : nullptr;
	if (!Arena)
	{
		return;
	}
	Clear();
	const EIJPPaletteRole Colour = Paddle->GetSide() == EIJPSide::Left ? EIJPPaletteRole::LeftPaddle : EIJPPaletteRole::RightPaddle;
	TArray<FVector2D> Placed;
	FActorSpawnParameters Params;
	Params.Owner = Arena;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	for (int32 i = 0; i < Count; ++i)
	{
		FVector2D Spot;
		if (!Arena->GetBonusTargets()->PickSpot(Size, CourtFraction, Spot, Placed))
		{
			continue;
		}
		if (AIJPBonusTarget* Bumper = Paddle->GetWorld()->SpawnActor<AIJPBonusTarget>(AIJPBonusTarget::StaticClass(), Arena->GetActorTransform(), Params))
		{
			Bumper->InitBumper(Arena, Spot, FVector2D(Size), Boost, Colour);
			Bumpers.Add(Bumper);
			Placed.Add(Spot);
		}
	}
	TimeLeft = Lifetime;
}

bool UIJPAbility_Bumpers::WantsAIUse() const
{
	const AIJPPaddle* Paddle = GetPaddle();
	const AIJPArena* Arena = Paddle ? Paddle->GetArena() : nullptr;
	return Bumpers.IsEmpty() && Arena && Arena->GetNumBallsInPlay() > 0;
}

void UIJPAbility_Bumpers::TickAbility(float DeltaSeconds)
{
	if (Bumpers.IsEmpty())
	{
		return;
	}
	TimeLeft -= DeltaSeconds;
	if (TimeLeft <= 0.f)
	{
		Clear();
	}
}

void UIJPAbility_Bumpers::Deactivate()
{
	Clear();
}

void UIJPAbility_Bumpers::Clear()
{
	for (AIJPBonusTarget* Bumper : Bumpers)
	{
		if (Bumper)
		{
			Bumper->Destroy();
		}
	}
	Bumpers.Reset();
	TimeLeft = 0.f;
}
