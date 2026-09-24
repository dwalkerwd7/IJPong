// It's Just Pong

#include "AI/IJPPaddleAIController.h"
#include "AI/IJPAIProfile.h"
#include "Core/IJPTypes.h"
#include "Gameplay/IJPArena.h"
#include "Gameplay/IJPBall.h"
#include "Gameplay/IJPPaddle.h"
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
	const AIJPArena* Arena = Paddle ? Paddle->GetArena() : nullptr;
	const AIJPBall* Ball = Arena ? Arena->GetBall() : nullptr;
	if (!Ball)
	{
		return;
	}

	// Only look at the ball every ReactionTime; steer toward the last decision every frame.
	DecisionTimer -= DeltaSeconds;
	if (DecisionTimer <= 0.f)
	{
		Decide(*Paddle, *Ball);
		DecisionTimer = FMath::Max(DecisionTimer + GetProfile().ReactionTime, 0.f);
	}

	Steer(*Paddle);
}

void AIJPPaddleAIController::Decide(const AIJPPaddle& Paddle, const AIJPBall& Ball)
{
	const UIJPAIProfile& P = GetProfile();
	const float GoalDir = IJP::SideSign(Paddle.GetSide());
	const FVector2D BallVelocity = Ball.GetPlaneVelocity();

	if (!Ball.IsInPlay() || BallVelocity.X * GoalDir <= 0.f)
	{
		// Heading away (or between points): drift back to the middle.
		bBallIncoming = false;
		TargetY = 0.f;
		return;
	}

	if (!bBallIncoming)
	{
		// A new shot is coming. Commit to one misjudgement and one aim for the whole approach,
		// so the paddle doesn't jitter between decisions.
		bBallIncoming = true;
		const float SpeedFraction = Ball.GetMaxSpeed() > 0.f ? BallVelocity.Size() / Ball.GetMaxSpeed() : 1.f;
		ShotError = Random.FRandRange(-1.f, 1.f) * P.ErrorSpread * SpeedFraction;
		ShotAim = Random.FRandRange(-1.f, 1.f) * P.AimSpread;
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

void AIJPPaddleAIController::Steer(AIJPPaddle& Paddle) const
{
	const UIJPAIProfile& P = GetProfile();
	const float Delta = TargetY - Paddle.GetPlanePosition().Y;
	if (FMath::Abs(Delta) <= P.ArrivalTolerance)
	{
		return;
	}

	const float Scale = bBallIncoming ? P.SpeedScale : P.IdleSpeedScale;
	Paddle.AddMoveInput(FMath::Clamp(Delta / P.SlowRadius, -1.f, 1.f) * Scale);
}
