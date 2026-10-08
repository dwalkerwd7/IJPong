// It's Just Pong

#include "Gameplay/IJPBonusTargetComponent.h"
#include "Audio/IJPToneSet.h"
#include "Audio/IJPToneSynthComponent.h"
#include "Engine/World.h"
#include "Era/IJPEra.h"
#include "Era/IJPEraSubsystem.h"
#include "Gameplay/IJPArena.h"
#include "Gameplay/IJPBall.h"
#include "Gameplay/IJPBonusTarget.h"

namespace
{
	/** New targets keep this far from any ball in play, so one never appears on top of a ball. */
	constexpr float TargetBallClearance = 60.f;
}

UIJPBonusTargetComponent::UIJPBonusTargetComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UIJPBonusTargetComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	const UIJPEra* Era = UIJPEraSubsystem::GetCurrentEra(this);
	const AIJPArena* Arena = GetArena();
	if (!Era || !Era->BonusTargets.bEnabled || !Arena || Arena->GetNumBallsInPlay() == 0)
	{
		return;
	}
	const FIJPBonusTargets& Settings = Era->BonusTargets;
	SpawnCountdown -= DeltaTime;
	if (SpawnCountdown > 0.f)
	{
		return;
	}
	SpawnCountdown = Settings.SpawnInterval;
	FVector2D Spot;
	if (Targets.Num() < Settings.MaxTargets && PickSpot(Spot))
	{
		SpawnTargetAt(Spot);
	}
}

AIJPBonusTarget* UIJPBonusTargetComponent::SpawnTargetAt(const FVector2D& Centre)
{
	AIJPArena* Arena = GetArena();
	if (!Arena)
	{
		return nullptr;
	}
	const UIJPEra* Era = UIJPEraSubsystem::GetCurrentEra(this);
	FActorSpawnParameters Params;
	Params.Owner = Arena;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AIJPBonusTarget* Target = GetWorld()->SpawnActor<AIJPBonusTarget>(AIJPBonusTarget::StaticClass(), Arena->GetActorTransform(), Params);
	if (Target)
	{
		Target->Init(Arena, Centre, Era ? Era->BonusTargets.Size : FIJPBonusTargets().Size);
		Targets.Add(Target);
	}
	return Target;
}

void UIJPBonusTargetComponent::Clear()
{
	for (AIJPBonusTarget* Target : Targets)
	{
		if (Target)
		{
			Target->Destroy();
		}
	}
	Targets.Reset();
	const UIJPEra* Era = UIJPEraSubsystem::GetCurrentEra(this);
	SpawnCountdown = Era ? Era->BonusTargets.SpawnInterval : 0.f;
}

void UIJPBonusTargetComponent::HandleTargetBroken(AIJPBonusTarget* Target, AIJPBall* Ball)
{
	Targets.Remove(Target);
	Target->Destroy();

	AIJPArena* Arena = GetArena();
	if (Arena)
	{
		Arena->GetTones()->PlayTone(Arena->GetToneSet().Pop);
	}
	EIJPSide Returner;
	if (Ball && Ball->GetLastReturner(Returner))
	{
		const UIJPEra* Era = UIJPEraSubsystem::GetCurrentEra(this);
		OnTargetHit.Broadcast(Returner, Era ? Era->BonusTargets.Coins : FIJPBonusTargets().Coins);
	}
}

AIJPArena* UIJPBonusTargetComponent::GetArena() const
{
	return Cast<AIJPArena>(GetOwner());
}

bool UIJPBonusTargetComponent::PickSpot(FVector2D& OutCentre) const
{
	const AIJPArena* Arena = GetArena();
	const UIJPEra* Era = UIJPEraSubsystem::GetCurrentEra(this);
	if (!Arena || !Era)
	{
		return false;
	}
	const FIJPBonusTargets& Settings = Era->BonusTargets;
	const FVector2D Half = Arena->GetHalfExtents();
	const float MaxX = FMath::Max(Half.X * Settings.CourtFraction - Settings.Size, 0.f);
	const float MaxY = FMath::Max(Half.Y - Settings.Size * 2.f, 0.f);
	for (int32 Try = 0; Try < 8; ++Try)
	{
		const FVector2D Spot(FMath::FRandRange(-MaxX, MaxX), FMath::FRandRange(-MaxY, MaxY));
		bool bClear = true;
		for (const AIJPBall* Ball : Arena->GetBalls())
		{
			bClear &= !Ball->IsInPlay() || FVector2D::Distance(Ball->GetPlanePosition(), Spot) > TargetBallClearance + Settings.Size;
		}
		for (const AIJPBonusTarget* Other : Targets)
		{
			bClear &= !Other || FVector2D::Distance(Other->GetPlanePosition(), Spot) > Settings.Size * 2.f;
		}
		if (bClear)
		{
			OutCentre = Spot;
			return true;
		}
	}
	return false;
}
