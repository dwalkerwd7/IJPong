// It's Just Pong

#include "Gameplay/IJPDriftingBlockComponent.h"
#include "Engine/World.h"
#include "Era/IJPEra.h"
#include "Era/IJPEraSubsystem.h"
#include "Gameplay/IJPArena.h"
#include "Gameplay/IJPBall.h"
#include "Gameplay/IJPBonusTarget.h"

UIJPDriftingBlockComponent::UIJPDriftingBlockComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UIJPDriftingBlockComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	const UIJPEra* Era = UIJPEraSubsystem::GetCurrentEra(this);
	const AIJPArena* Arena = GetArena();
	if (!Era || !Arena || !Arena->IsTwistOn(EIJPTwist::DriftingBlocks))
	{
		if (!Blocks.IsEmpty())
		{
			Reset();
		}
		return;
	}
	if (Blocks.IsEmpty())
	{
		Place();
	}

	const FIJPDriftingBlocks& Settings = Era->DriftingBlocks;
	const float Limit = Arena->GetHalfExtents().Y - Settings.Size.Y * 0.5f;
	for (int32 i = 0; i < Blocks.Num(); ++i)
	{
		AIJPBonusTarget* Block = Blocks[i];
		if (!Block)
		{
			continue;
		}
		const FVector2D From = Block->GetPlanePosition();
		FVector2D To = From + FVector2D(0.f, Directions[i] * Settings.Speed * DeltaTime);
		if (FMath::Abs(To.Y) > Limit)
		{
			// Turn back at the wall.
			To.Y = FMath::Clamp(To.Y, -Limit, Limit);
			Directions[i] = -Directions[i];
		}
		// Never into a ball: wait for it to pass.
		const FVector2D Half = Block->GetSize() * 0.5f;
		bool bBlocked = false;
		for (const AIJPBall* Ball : Arena->GetBalls())
		{
			const float Reach = Ball->GetSize() * 0.5f + 1.f;
			const FVector2D Delta = Ball->GetPlanePosition() - To;
			bBlocked |= Ball->IsInPlay() && FMath::Abs(Delta.X) < Half.X + Reach && FMath::Abs(Delta.Y) < Half.Y + Reach;
		}
		if (!bBlocked)
		{
			Block->SetPlanePosition(To);
		}
	}
}

void UIJPDriftingBlockComponent::Reset()
{
	for (AIJPBonusTarget* Block : Blocks)
	{
		if (Block)
		{
			Block->Destroy();
		}
	}
	Blocks.Reset();
	Directions.Reset();
}

AIJPArena* UIJPDriftingBlockComponent::GetArena() const
{
	return Cast<AIJPArena>(GetOwner());
}

void UIJPDriftingBlockComponent::Place()
{
	AIJPArena* Arena = GetArena();
	const UIJPEra* Era = UIJPEraSubsystem::GetCurrentEra(this);
	if (!Arena || !Era)
	{
		return;
	}
	const FIJPDriftingBlocks& Settings = Era->DriftingBlocks;
	const FVector2D Half = Arena->GetHalfExtents();
	const float Reach = Half.X * Settings.CourtFraction;
	const float Limit = FMath::Max(Half.Y - Settings.Size.Y * 0.5f, 0.f);
	FActorSpawnParameters Params;
	Params.Owner = Arena;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	for (int32 i = 0; i < Settings.Count; ++i)
	{
		// Alternating sides of the net, pairs stepping inward; never on the net line, where balls are served.
		const float X = (i % 2 == 0 ? -1.f : 1.f) * Reach * FMath::Max(1.f - 0.4f * (i / 2), 0.2f);
		const FVector2D Centre(X, FMath::FRandRange(-Limit, Limit));
		AIJPBonusTarget* Block = GetWorld()->SpawnActor<AIJPBonusTarget>(AIJPBonusTarget::StaticClass(), Arena->GetActorTransform(), Params);
		if (Block)
		{
			Block->InitBumper(Arena, Centre, Settings.Size, 1.f, EIJPPaletteRole::Walls);
			Blocks.Add(Block);
			Directions.Add(i % 2 == 0 ? 1.f : -1.f);
		}
	}
}
