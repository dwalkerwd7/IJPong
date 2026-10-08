// It's Just Pong

#include "Abilities/IJPAbility_Barrier.h"
#include "Core/IJPGameModeBase.h"
#include "Engine/World.h"
#include "Gameplay/IJPArena.h"
#include "Gameplay/IJPBall.h"
#include "Gameplay/IJPMatchComponent.h"
#include "Gameplay/IJPPaddle.h"
#include "Gameplay/IJPPongMath.h"

namespace IJPBarrierUpgrades
{
	const FName DurationUpgrade(TEXT("Duration"));
	const FName LastStandUpgrade(TEXT("LastStand"));
	const FName MendingUpgrade(TEXT("Mending"));
	const FName ReboundUpgrade(TEXT("Rebound"));

	/** Last Stand looks this far ahead for a ball the paddle won't reach. */
	constexpr float LastStandLead = 0.12f;
}

void UIJPAbility_Barrier::Activate()
{
	Raise(Duration + GetUpgrade(IJPBarrierUpgrades::DurationUpgrade));
}

void UIJPAbility_Barrier::TickAbility(float DeltaSeconds)
{
	if (TimeLeft > 0.f)
	{
		TimeLeft -= DeltaSeconds;
		if (TimeLeft <= 0.f)
		{
			Deactivate();
		}
		return;
	}

	// Last Stand: once a match, up by itself when a goal is coming.
	if (GetUpgrade(IJPBarrierUpgrades::LastStandUpgrade) > 0.f && !HasUsedLastStand() && IsGoalComing())
	{
		LastStandMatch = GetMatchNumber();
		Raise(LastStandTime);
	}
}

void UIJPAbility_Barrier::Deactivate()
{
	TimeLeft = 0.f;
	SetUp(false);
}

bool UIJPAbility_Barrier::HasUsedLastStand() const
{
	return LastStandMatch != 0 && LastStandMatch == GetMatchNumber();
}

void UIJPAbility_Barrier::Raise(float Seconds)
{
	TimeLeft = FMath::Max(TimeLeft, Seconds);
	SetUp(true);
}

void UIJPAbility_Barrier::HandleBarrierHit(EIJPSide Side, AIJPBall* Ball)
{
	const AIJPPaddle* Paddle = GetPaddle();
	if (TimeLeft <= 0.f || !Paddle || Side != Paddle->GetSide() || !Ball)
	{
		return;
	}
	if (GetUpgrade(IJPBarrierUpgrades::ReboundUpgrade) > 0.f)
	{
		Ball->Boost(ReboundBoost);
	}
	const float Mending = GetUpgrade(IJPBarrierUpgrades::MendingUpgrade);
	const AIJPGameModeBase* GameMode = Paddle->GetWorld()->GetAuthGameMode<AIJPGameModeBase>();
	if (UIJPMatchComponent* Match = Mending > 0.f && GameMode ? GameMode->GetMatch() : nullptr)
	{
		Match->Heal(Side, Mending);
	}
}

void UIJPAbility_Barrier::SetUp(bool bUp)
{
	const AIJPPaddle* Paddle = GetPaddle();
	if (AIJPArena* Arena = Paddle ? Paddle->GetArena() : nullptr)
	{
		Arena->SetBarrierUp(Paddle->GetSide(), bUp);
		if (bUp)
		{
			Arena->OnBarrierHit.AddUniqueDynamic(this, &UIJPAbility_Barrier::HandleBarrierHit);
		}
		else
		{
			Arena->OnBarrierHit.RemoveDynamic(this, &UIJPAbility_Barrier::HandleBarrierHit);
		}
	}
}

bool UIJPAbility_Barrier::IsGoalComing() const
{
	const AIJPPaddle* Paddle = GetPaddle();
	const AIJPArena* Arena = Paddle ? Paddle->GetArena() : nullptr;
	if (!Arena)
	{
		return false;
	}
	const float GoalDir = IJP::SideSign(Paddle->GetSide());
	const float LaneX = Paddle->GetPlanePosition().X;
	for (const AIJPBall* Ball : Arena->GetBalls())
	{
		const FVector2D Position = Ball->GetPlanePosition();
		const FVector2D Velocity = Ball->GetPlaneVelocity();
		if (!Ball->IsInPlay() || Ball->IsHeld() || Velocity.X * GoalDir <= 0.f)
		{
			continue;
		}
		// Already past the paddle's lane: nothing else will stop it.
		if ((Position.X - LaneX) * GoalDir > 0.f)
		{
			return true;
		}
		// About to reach the lane where the paddle isn't.
		const float TimeToLane = (LaneX - Position.X) / Velocity.X;
		const float BallLimit = Arena->GetHalfExtents().Y - Ball->GetSize() * 0.5f;
		float ArriveY = 0.f;
		if (TimeToLane <= IJPBarrierUpgrades::LastStandLead && FIJPPongMath::PredictInterceptY(Position, Velocity, LaneX, -BallLimit, BallLimit, ArriveY))
		{
			float CentreY = 0.f;
			float HalfLength = 0.f;
			Paddle->GetHitSpan(ArriveY, CentreY, HalfLength);
			if (FMath::Abs(ArriveY - CentreY) > HalfLength + Ball->GetSize() * 0.5f)
			{
				return true;
			}
		}
	}
	return false;
}

int32 UIJPAbility_Barrier::GetMatchNumber() const
{
	const AIJPPaddle* Paddle = GetPaddle();
	const AIJPGameModeBase* GameMode = Paddle ? Paddle->GetWorld()->GetAuthGameMode<AIJPGameModeBase>() : nullptr;
	const UIJPMatchComponent* Match = GameMode ? GameMode->GetMatch() : nullptr;
	return Match ? Match->GetMatchNumber() : 0;
}
