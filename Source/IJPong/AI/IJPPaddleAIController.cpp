// It's Just Pong

#include "AI/IJPPaddleAIController.h"
#include "Era/IJPEraSubsystem.h"
#include "Era/IJPEra.h"
#include "Abilities/IJPAbilityComponent.h"
#include "AI/IJPAIProfile.h"
#include "Core/IJPTypes.h"
#include "Gameplay/IJPArena.h"
#include "Gameplay/IJPBall.h"
#include "Gameplay/IJPPaddle.h"
#include "Gameplay/IJPSpellStrike.h"
#include "Gameplay/IJPPongMath.h"

AIJPPaddleAIController::AIJPPaddleAIController()
{
	PrimaryActorTick.bCanEverTick = true;
	Random.GenerateNewSeed();
}

void AIJPPaddleAIController::SetProfile(const UIJPAIProfile* InProfile)
{
	Profile = InProfile;
}

const UIJPAIProfile& AIJPPaddleAIController::GetProfile() const
{
	return Profile ? *Profile : *GetDefault<UIJPAIProfile>();
}

void AIJPPaddleAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	// Player controllers make their pawn tick after them; AI controllers don't, so do it here.
	// Otherwise the paddle could consume this frame's input before we've given it.
	InPawn->AddTickPrerequisiteActor(this);

	DecisionTimer = 0.f;
	TargetY = 0.f;
	bBallIncoming = false;
}

void AIJPPaddleAIController::OnUnPossess()
{
	if (APawn* PossessedPawn = GetPawn())
	{
		PossessedPawn->RemoveTickPrerequisiteActor(this);
	}
	Super::OnUnPossess();
}

void AIJPPaddleAIController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	AIJPPaddle* Paddle = GetPawn<AIJPPaddle>();
	if (!Paddle || !Paddle->GetArena())
	{
		return;
	}

	// A read shot overrides the balls; otherwise only look at them every ReactionTime and steer
	// toward the last decision every frame.
	DecisionTimer -= DeltaSeconds;
	if (bHasReadTarget)
	{
		TargetY = ReadTarget;
		bBallIncoming = true; // move with purpose, not at the idle drift
	}
	else if (DecisionTimer <= 0.f)
	{
		Decide(*Paddle, PickIncomingBall(*Paddle));
		DecisionTimer = FMath::Max(DecisionTimer + GetReactionTime(), 0.f);
	}

	Steer(*Paddle);
	UseAbilities(*Paddle);
}

float AIJPPaddleAIController::GetReactionTime() const
{
	// The profile's time at this skill, quickened (or slowed) by the era.
	const UIJPEra* Era = UIJPEraSubsystem::GetCurrentEra(this);
	return GetProfile().ReactionTime.At(Skill) * (Era ? Era->AIReactionScale : 1.f);
}

void AIJPPaddleAIController::ClearReadTarget()
{
	if (bHasReadTarget)
	{
		bHasReadTarget = false;
		bBallIncoming = false;
		TrackedBall = nullptr;
		DecisionTimer = 0.f; // look at the balls right away
	}
}

void AIJPPaddleAIController::UseAbilities(AIJPPaddle& Paddle) const
{
	// Each ability knows its own moment; the AI just presses the button when it says so.
	UIJPAbilityComponent* Abilities = Paddle.GetAbilities();
	for (int32 i = 0; i < static_cast<int32>(EIJPAbilitySlot::Count); ++i)
	{
		const EIJPAbilitySlot Slot = static_cast<EIJPAbilitySlot>(i);
		const UIJPAbility* Ability = Abilities->GetAbility(Slot);
		if (Ability && Abilities->IsReady(Slot) && Ability->WantsAIUse())
		{
			Abilities->TryActivate(Slot);
		}
	}
}

const AIJPBall* AIJPPaddleAIController::PickIncomingBall(const AIJPPaddle& Paddle) const
{
	const float GoalDir = IJP::SideSign(Paddle.GetSide());
	const float LaneX = Paddle.GetPlanePosition().X;
	const AIJPBall* Soonest = nullptr;
	float SoonestTime = TNumericLimits<float>::Max();
	for (const AIJPBall* Ball : Paddle.GetArena()->GetBalls())
	{
		const FVector2D Velocity = Ball->GetPlaneVelocity();
		if (!Ball->IsInPlay() || Velocity.X * GoalDir <= 0.f)
		{
			continue;
		}
		// Time to reach the paddle's lane, horizontally; walls don't change that.
		const float Time = (LaneX - Ball->GetPlanePosition().X) / Velocity.X;
		if (Time >= 0.f && Time < SoonestTime)
		{
			Soonest = Ball;
			SoonestTime = Time;
		}
	}
	return Soonest;
}

void AIJPPaddleAIController::Decide(const AIJPPaddle& Paddle, const AIJPBall* IncomingBall)
{
	if (!IncomingBall)
	{
		// Nothing heading this way (or between points): drift back to the middle.
		bBallIncoming = false;
		TrackedBall = nullptr;
		TargetY = 0.f;
		return;
	}

	const AIJPBall& Ball = *IncomingBall;
	const UIJPAIProfile& P = GetProfile();
	const float GoalDir = IJP::SideSign(Paddle.GetSide());
	const FVector2D BallVelocity = Ball.GetPlaneVelocity();

	if (!bBallIncoming || TrackedBall != IncomingBall)
	{
		// A new shot is coming (or a different ball became the most urgent). Commit to one
		// misjudgement and one aim for its whole approach, so the paddle doesn't jitter between decisions.
		bBallIncoming = true;
		TrackedBall = IncomingBall;
		const float SpeedFraction = Ball.GetMaxSpeed() > 0.f ? BallVelocity.Size() / Ball.GetMaxSpeed() : 1.f;
		ShotError = Random.FRandRange(-1.f, 1.f) * P.ErrorSpread.At(Skill) * SpeedFraction;
		ShotAim = Random.FRandRange(-1.f, 1.f) * P.AimSpread.At(Skill);
	}

	const FVector2D HalfExtents = Paddle.GetArena()->GetHalfExtents();
	const float BallHalf = Ball.GetSize() * 0.5f;
	const float PaddleHalfHeight = Paddle.GetSize().Y * 0.5f;
	const float Reach = PaddleHalfHeight + BallHalf;
	const float FaceX = Paddle.GetPlanePosition().X - GoalDir * (Paddle.GetSize().X * 0.5f + BallHalf);
	const float BallLimitY = HalfExtents.Y - BallHalf;

	float InterceptY = 0.f;
	if (FIJPPongMath::PredictInterceptY(Ball.GetPlanePosition(), BallVelocity, FaceX, -BallLimitY, BallLimitY, InterceptY))
	{
		// Aim > 0 puts the paddle below the ball, so the ball meets its upper half and goes back upward.
		const float PaddleLimitY = HalfExtents.Y - PaddleHalfHeight;
		TargetY = FMath::Clamp(InterceptY + ShotError - ShotAim * Reach, -PaddleLimitY, PaddleLimitY);
	}
	// Otherwise the ball is already past the face: keep the last target.
}

float AIJPPaddleAIController::AvoidStrikes(const AIJPPaddle& Paddle, float Target) const
{
	// Stand just outside any zone about to land here (the edge on the side it's on, or the other one at a wall).
	const float Half = Paddle.GetSize().Y * 0.5f;
	const float Limit = Paddle.GetArena()->GetHalfExtents().Y - Half;
	for (const TWeakObjectPtr<AIJPSpellStrike>& Weak : Paddle.GetArena()->GetStrikes())
	{
		const AIJPSpellStrike* Strike = Weak.Get();
		if (!Strike || Strike->HasLanded() || Strike->GetTargetSide() != Paddle.GetSide())
		{
			continue;
		}
		const float Clear = Strike->GetHalfHeight() + Half + 6.f;
		const float Above = Strike->GetTargetY() + Clear;
		const float Below = Strike->GetTargetY() - Clear;
		if (FMath::Abs(Target - Strike->GetTargetY()) < Clear)
		{
			const bool bAboveFits = Above <= Limit;
			const bool bBelowFits = Below >= -Limit;
			// Out on the side it's already on (crossing the zone to the other edge could take too long).
			const bool bGoAbove = bAboveFits && (!bBelowFits || Paddle.GetPlanePosition().Y >= Strike->GetTargetY());
			Target = bGoAbove ? Above : Below;
			continue;
		}

		// Heading for the far side: only cross if it can get all the way over before it lands;
		// otherwise wait at the near edge.
		const float Current = Paddle.GetPlanePosition().Y;
		const bool bCrossesUp = Current < Above && Target >= Above;
		const bool bCrossesDown = Current > Below && Target <= Below;
		if (bCrossesUp || bCrossesDown)
		{
			const float Distance = FMath::Abs((bCrossesUp ? Above : Below) - Current);
			const float TimeToCross = Distance / FMath::Max(Paddle.GetMaxSpeed(), 1.f) + 0.1f;
			if (TimeToCross > Strike->GetTimeLeft())
			{
				Target = Current >= Strike->GetTargetY() ? Above : Below;
				Target = FMath::Clamp(Target, -Limit, Limit);
			}
		}
	}
	return Target;
}

void AIJPPaddleAIController::Steer(AIJPPaddle& Paddle) const
{
	const UIJPAIProfile& P = GetProfile();
	// Split in two: put the nearer half, not the gap, where the ball will be.
	float Aim = TargetY;
	if (const float HalfOffset = Paddle.GetSplitHalfOffset(); HalfOffset > 0.f)
	{
		const float Current = Paddle.GetPlanePosition().Y;
		const float Limit = Paddle.GetArena()->GetHalfExtents().Y - Paddle.GetSize().Y * 0.5f;
		const float Below = FMath::Clamp(TargetY - HalfOffset, -Limit, Limit); // top half on the ball
		const float Above = FMath::Clamp(TargetY + HalfOffset, -Limit, Limit); // bottom half on the ball
		Aim = FMath::Abs(Below - Current) <= FMath::Abs(Above - Current) ? Below : Above;
	}
	const float Target = AvoidStrikes(Paddle, Aim);
	const float Delta = Target - Paddle.GetPlanePosition().Y;

	// Dodging a spell is urgent: full purpose, not the idle drift, only a short ease-in, and "close enough"
	// isn't (stopping short of the edge of the zone is still a hit).
	const bool bDodging = Target != Aim;
	if (FMath::Abs(Delta) <= (bDodging ? 1.f : P.ArrivalTolerance))
	{
		return;
	}
	const float Scale = (bBallIncoming || bDodging ? P.SpeedScale : P.IdleSpeedScale).At(Skill);
	const float Push = FMath::Clamp(Delta / (bDodging ? 8.f : P.SlowRadius), -1.f, 1.f);
	Paddle.AddMoveInput(Push * Scale);
}
