// It's Just Pong

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Core/IJPGameMode.h"
#include "Core/IJPTypes.h"
#include "Engine/World.h"
#include "Gameplay/IJPArena.h"
#include "Gameplay/IJPBall.h"
#include "Gameplay/IJPPaddle.h"
#include "Gameplay/IJPPongMath.h"
#include "Tests/IJPTestWorld.h"

namespace IJPBallTests
{
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter;

	// Rotated and offset so every check also exercises the plane <-> world mapping.
	const FTransform ArenaTransform(FRotator(0.f, -20.f, 0.f), FVector(-300.f, 800.f, 100.f));

	/** Furthest the ball's centre can legitimately be from the middle, with a little slack for sweep pull-back. */
	FVector2D CentreLimits(const AIJPArena* Arena, const AIJPBall* Ball)
	{
		const float Half = Ball->GetSize() * 0.5f;
		return FVector2D(Arena->GetHalfExtents().X + Half, Arena->GetHalfExtents().Y - Half) + FVector2D(0.5f);
	}

	/** Step until Condition is true or MaxSeconds pass. Returns whether Condition became true. */
	bool RunUntil(FIJPTestWorld& Test, float MaxSeconds, TFunctionRef<bool()> Condition, TFunctionRef<void()> BeforeEachStep = [] {})
	{
		const int32 Steps = FMath::CeilToInt(MaxSeconds / FIJPTestWorld::FixedStep);
		for (int32 i = 0; i < Steps && !Condition(); ++i)
		{
			BeforeEachStep();
			Test.Step();
		}
		return Condition();
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPBallWallTest, "IJPong.Ball.BouncesOffWalls", IJPBallTests::Flags)
bool FIJPBallWallTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test(IJPBallTests::ArenaTransform);
	AIJPArena* Arena = Test.GetArena();
	AIJPBall* Ball = Arena->GetBall();
	UTEST_NOT_NULL("Ball", Ball);
	UTEST_FALSE("Ball waits until served", Ball->IsInPlay());

	// 45 degrees up toward the right: reaches the top wall well before the right paddle's lane.
	Ball->Serve(EIJPSide::Right, 45.f);
	const FVector2D ServeVelocity = Ball->GetPlaneVelocity();
	const FVector2D Limits = IJPBallTests::CentreLimits(Arena, Ball);

	bool bStayedInside = true;
	const bool bBounced = IJPBallTests::RunUntil(Test, 2.f, [&]
	{
		bStayedInside &= FMath::Abs(Ball->GetPlanePosition().Y) <= Limits.Y;
		return Ball->GetPlaneVelocity().Y < 0.f;
	});

	UTEST_TRUE("Bounced off the top wall", bBounced);
	UTEST_TRUE("Never went through the wall", bStayedInside);
	UTEST_TRUE("Still in play", Ball->IsInPlay());
	UTEST_EQUAL_TOLERANCE("Wall keeps horizontal velocity", Ball->GetPlaneVelocity().X, ServeVelocity.X, 1e-3);
	UTEST_EQUAL_TOLERANCE("Wall mirrors vertical velocity", Ball->GetPlaneVelocity().Y, -ServeVelocity.Y, 1e-3);
	UTEST_EQUAL("Walls don't count as rally hits", Ball->GetRallyHits(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPBallPaddleCentreTest, "IJPong.Ball.PaddleCentreReturnsStraightAndFaster", IJPBallTests::Flags)
bool FIJPBallPaddleCentreTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test(IJPBallTests::ArenaTransform);
	AIJPBall* Ball = Test.GetArena()->GetBall();
	UTEST_NOT_NULL("Ball", Ball);

	// Straight at the (centred, idle) left paddle.
	Ball->Serve(EIJPSide::Left, 0.f);
	const float ServeSpeed = Ball->GetPlaneVelocity().Size();

	UTEST_TRUE("Came back off the paddle", IJPBallTests::RunUntil(Test, 2.f, [Ball] { return Ball->GetPlaneVelocity().X > 0.f || !Ball->IsInPlay(); }));
	UTEST_TRUE("Still in play", Ball->IsInPlay());
	UTEST_EQUAL("One rally hit", Ball->GetRallyHits(), 1);
	UTEST_EQUAL_TOLERANCE("Dead-centre hit returns flat", Ball->GetPlaneVelocity().Y, 0.0, 1e-3);
	UTEST_TRUE("Paddle hit speeds the ball up", Ball->GetPlaneVelocity().Size() > ServeSpeed);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPBallPaddleAngleTest, "IJPong.Ball.PaddleOffsetSetsBounceAngle", IJPBallTests::Flags)
bool FIJPBallPaddleAngleTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test(IJPBallTests::ArenaTransform);
	AIJPBall* Ball = Test.GetArena()->GetBall();
	AIJPPaddle* Paddle = Test.GetArena()->GetPaddle(EIJPSide::Left);
	UTEST_NOT_NULL("Ball", Ball);
	UTEST_NOT_NULL("Paddle", Paddle);

	// Nudge the paddle up a little and let it settle, so a flat serve meets its lower half.
	Test.RunFor(2.f * FIJPTestWorld::FixedStep, [Paddle] { Paddle->AddMoveInput(1.f); });
	Test.RunFor(0.2f);
	const float PaddleY = Paddle->GetPlanePosition().Y;
	UTEST_TRUE("Paddle sits above centre", PaddleY > 5.f);

	Ball->Serve(EIJPSide::Left, 0.f);
	UTEST_TRUE("Came back off the paddle", IJPBallTests::RunUntil(Test, 2.f, [Ball] { return Ball->GetPlaneVelocity().X > 0.f || !Ball->IsInPlay(); }));
	UTEST_EQUAL("One rally hit", Ball->GetRallyHits(), 1);

	// The ball's centre was at Y=0, so it met the paddle PaddleY below the paddle's centre.
	const float Reach = Paddle->GetSize().Y * 0.5f + Ball->GetSize() * 0.5f;
	const float ExpectedDeg = (-PaddleY / Reach) * Ball->GetMaxBounceAngle();
	const FVector2D V = Ball->GetPlaneVelocity();
	const float ActualDeg = FMath::RadiansToDegrees(FMath::Atan2(V.Y, V.X));
	UTEST_TRUE("Lower half sends the ball down", V.Y < 0.f);
	UTEST_EQUAL_TOLERANCE("Angle follows hit offset", ActualDeg, ExpectedDeg, 0.5f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPBallGoalTest, "IJPong.Ball.GoalScoresAndReserves", IJPBallTests::Flags)
bool FIJPBallGoalTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test(IJPBallTests::ArenaTransform);
	AIJPArena* Arena = Test.GetArena();
	AIJPBall* Ball = Arena->GetBall();
	AIJPPaddle* Paddle = Arena->GetPaddle(EIJPSide::Left);
	AIJPGameMode* GameMode = Cast<AIJPGameMode>(Test.GetWorld()->GetAuthGameMode());
	UTEST_NOT_NULL("Ball", Ball);
	UTEST_NOT_NULL("GameMode", GameMode);

	// Get the left paddle out of the way (pinned to the top wall), then serve straight at its goal.
	// Serving before the game mode's own first serve (at ServeDelay) means that one is skipped.
	Test.RunFor(0.5f, [Paddle] { Paddle->AddMoveInput(1.f); });
	Ball->Serve(EIJPSide::Left, 0.f);

	UTEST_TRUE("Ball went into the goal", IJPBallTests::RunUntil(Test, 2.f, [Ball] { return !Ball->IsInPlay(); }));
	UTEST_EQUAL("No rally hits", Ball->GetRallyHits(), 0);
	UTEST_EQUAL("Right scored", GameMode->GetScore(EIJPSide::Right), 1);
	UTEST_EQUAL("Left didn't", GameMode->GetScore(EIJPSide::Left), 0);
	UTEST_TRUE("Ball hidden between points", Ball->IsHidden());

	// After the serve delay, the side that conceded receives the next serve.
	UTEST_TRUE("Re-served", IJPBallTests::RunUntil(Test, 2.f, [Ball] { return Ball->IsInPlay(); }));
	UTEST_TRUE("Served toward the side that conceded", Ball->GetPlaneVelocity().X < 0.f);
	UTEST_FALSE("Ball visible in play", Ball->IsHidden());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPBallLongRallyTest, "IJPong.Ball.LongRallyAtMaxSpeedNoTunnelling", IJPBallTests::Flags)
bool FIJPBallLongRallyTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test(IJPBallTests::ArenaTransform);
	AIJPArena* Arena = Test.GetArena();
	AIJPBall* Ball = Arena->GetBall();
	UTEST_NOT_NULL("Ball", Ball);
	const FVector2D Limits = IJPBallTests::CentreLimits(Arena, Ball);
	const float BallHalf = Ball->GetSize() * 0.5f;

	// A near-perfect player on both sides: predict where the ball meets the paddle face and stand so it
	// hits off-centre, alternating up and down, so the rally keeps crossing the walls at steep angles.
	auto Drive = [&]
	{
		for (EIJPSide Side : { EIJPSide::Left, EIJPSide::Right })
		{
			AIJPPaddle* Paddle = Arena->GetPaddle(Side);
			const float Sign = IJP::SideSign(Side);
			const float FaceX = Paddle->GetPlanePosition().X - Sign * (Paddle->GetSize().X * 0.5f + BallHalf);
			const float Reach = Paddle->GetSize().Y * 0.5f + BallHalf;
			const float Aim = (Ball->GetRallyHits() % 2 == 0 ? 0.4f : -0.4f) * Reach;

			float TargetY = 0.f;
			float InterceptY = 0.f;
			if (FIJPPongMath::PredictInterceptY(Ball->GetPlanePosition(), Ball->GetPlaneVelocity(), FaceX, -Limits.Y, Limits.Y, InterceptY))
			{
				TargetY = InterceptY - Aim;
			}
			Paddle->AddMoveInput(FMath::Clamp((TargetY - Paddle->GetPlanePosition().Y) / 20.f, -1.f, 1.f));
		}
	};

	Ball->Serve(EIJPSide::Right, 20.f);

	constexpr int32 TargetHits = 30;
	bool bStayedInside = true;
	float TopSpeed = 0.f;
	IJPBallTests::RunUntil(Test, 60.f, [&]
	{
		const FVector2D P = Ball->GetPlanePosition();
		bStayedInside &= FMath::Abs(P.X) <= Limits.X && FMath::Abs(P.Y) <= Limits.Y;
		TopSpeed = FMath::Max(TopSpeed, static_cast<float>(Ball->GetPlaneVelocity().Size()));
		return !Ball->IsInPlay() || Ball->GetRallyHits() >= TargetHits;
	}, Drive);

	UTEST_TRUE("No goal during the rally", Ball->IsInPlay());
	UTEST_TRUE("Rally reached the target hit count", Ball->GetRallyHits() >= TargetHits);
	UTEST_EQUAL_TOLERANCE("Reached MaxSpeed", TopSpeed, Ball->GetMaxSpeed(), 1e-2f);
	UTEST_TRUE("Ball never left the field", bStayedInside);
	return true;
}

#endif
