// It's Just Pong

#include "Abilities/IJPAbility_Split.h"
#include "Gameplay/IJPArena.h"
#include "Gameplay/IJPBall.h"
#include "Gameplay/IJPPaddle.h"

namespace IJPSplitUpgrades
{
	const FName SpreadUpgrade(TEXT("Spread"));
	const FName TripleUpgrade(TEXT("Triple"));
	const FName SmashUpgrade(TEXT("Smash"));
	const FName DecoyUpgrade(TEXT("Decoy"));
}

void UIJPAbility_Split::OnBallHit(AIJPBall& Ball)
{
	const AIJPPaddle* Paddle = GetPaddle();
	AIJPArena* Arena = Paddle ? Paddle->GetArena() : nullptr;
	if (!bArmed || !Arena)
	{
		return;
	}
	bArmed = false;

	const FVector2D Straight = Ball.GetPlaneVelocity();
	TArray<AIJPBall*> Pieces = { &Ball };
	AIJPBall* Twin = FanOut(Ball, *Arena, SpreadDeg + GetUpgrade(IJPSplitUpgrades::SpreadUpgrade));
	if (Twin)
	{
		Pieces.Add(Twin);
		if (GetUpgrade(IJPSplitUpgrades::DecoyUpgrade) > 0.f && DecoyType)
		{
			Twin->SetType(DecoyType);
		}
	}
	// Triple Split: a third down the middle, where the return was going.
	if (GetUpgrade(IJPSplitUpgrades::TripleUpgrade) > 0.f)
	{
		if (AIJPBall* Third = Arena->AddBall(&Ball.GetType()))
		{
			Third->Launch(Ball.GetPlanePosition(), Straight);
			Third->MarkSplit();
			Pieces.Add(Third);
		}
	}
	if (GetUpgrade(IJPSplitUpgrades::SmashUpgrade) > 0.f)
	{
		for (AIJPBall* Piece : Pieces)
		{
			Piece->Boost(SmashBoost);
		}
	}
}

AIJPBall* UIJPAbility_Split::FanOut(AIJPBall& Ball, AIJPArena& Arena, float Spread)
{
	// Fan the return out: this ball turns half the spread one way, a new one the other way.
	const FVector2D Velocity = Ball.GetPlaneVelocity();
	const float Speed = Velocity.Size();
	const float DirX = FMath::Sign(Velocity.X);
	const float Angle = FMath::RadiansToDegrees(FMath::Atan2(Velocity.Y, FMath::Abs(Velocity.X)));
	auto Heading = [&](float Deg)
	{
		const float Clamped = FMath::DegreesToRadians(FMath::Clamp(Deg, -Ball.GetMaxBounceAngle(), Ball.GetMaxBounceAngle()));
		return FVector2D(DirX * FMath::Cos(Clamped), FMath::Sin(Clamped)) * Speed;
	};

	Ball.SetPlaneVelocity(Heading(Angle + Spread * 0.5f));
	AIJPBall* Twin = Arena.AddBall(&Ball.GetType());
	if (Twin)
	{
		Twin->Launch(Ball.GetPlanePosition(), Heading(Angle - Spread * 0.5f));
	}
	return Twin;
}
