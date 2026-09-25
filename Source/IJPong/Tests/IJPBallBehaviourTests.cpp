// It's Just Pong

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Core/IJPTestGameMode.h"
#include "Core/IJPTypes.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "Gameplay/IJPArena.h"
#include "Gameplay/IJPBall.h"
#include "Gameplay/IJPBallType.h"
#include "Gameplay/IJPMatchComponent.h"
#include "Gameplay/IJPPaddle.h"
#include "Tests/IJPTestWorld.h"

namespace IJPBallBehaviourTests
{
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter;

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

	AIJPTestGameMode* GetMode(const FIJPTestWorld& Test)
	{
		return Cast<AIJPTestGameMode>(Test.GetWorld()->GetAuthGameMode());
	}

	/** Paddle held at the top, Type served straight into Side's goal. */
	bool ScoreAgainst(FIJPTestWorld& Test, AIJPArena* Arena, EIJPSide Side, const UIJPBallType* Type)
	{
		AIJPPaddle* Paddle = Arena->GetPaddle(Side);
		const auto HoldUp = [Paddle] { Paddle->AddMoveInput(1.f); };
		Test.RunFor(0.5f, HoldUp);
		AIJPBall* Ball = Arena->GetBall();
		Ball->SetType(Type);
		Ball->Serve(Side, 0.f);
		return RunUntil(Test, 3.f, [Ball] { return !Ball->IsInPlay(); }, HoldUp);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPGhostBallTest, "IJPong.Ball.GhostHidesMidCourt", IJPBallBehaviourTests::Flags)
bool FIJPGhostBallTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test;
	AIJPArena* Arena = Test.GetArena();
	UIJPBallType* Ghost = NewObject<UIJPBallType>(GetTransientPackage());
	Ghost->GhostBand = 0.4f;
	AIJPBall* Ball = Arena->GetBall();
	Ball->SetType(Ghost);
	const float BandX = 0.4f * Arena->GetHalfExtents().X;

	// From deep on the left, across the middle.
	Ball->Launch(FVector2D(-BandX - 60.f, 0.f), FVector2D(400.f, 0.f));
	Test.Step();
	UTEST_FALSE("Seen near the paddle", Ball->IsHidden());
	UTEST_TRUE("Gone in the middle", IJPBallBehaviourTests::RunUntil(Test, 1.f, [Ball] { return Ball->IsGhosted(); }));
	UTEST_TRUE("Really hidden", Ball->IsHidden());
	UTEST_TRUE("Still in play", Ball->IsInPlay());
	UTEST_TRUE("Back past the band", IJPBallBehaviourTests::RunUntil(Test, 2.f, [Ball, BandX] { return Ball->GetPlanePosition().X > BandX + 5.f; }));
	UTEST_FALSE("Seen again", Ball->IsHidden());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPTwinBallTest, "IJPong.Ball.TwinSplitsOnItsFirstReturnOnly", IJPBallBehaviourTests::Flags)
bool FIJPTwinBallTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test;
	AIJPArena* Arena = Test.GetArena();
	Arena->GetPaddle(EIJPSide::Right)->GetController()->UnPossess();
	UIJPBallType* Twin = NewObject<UIJPBallType>(GetTransientPackage());
	Twin->bSplitsOnFirstHit = true;
	Twin->SplitSpread = 20.f;
	AIJPBall* Ball = Arena->GetBall();
	Ball->SetType(Twin);

	// Straight at the (still, centred) left paddle.
	Ball->Serve(EIJPSide::Left, 0.f);
	UTEST_TRUE("Returned", IJPBallBehaviourTests::RunUntil(Test, 2.f, [Ball] { return Ball->GetPlaneVelocity().X > 0.f; }));
	UTEST_EQUAL("Two now", Arena->GetNumBallsInPlay(), 2);
	for (const AIJPBall* Each : Arena->GetBalls())
	{
		if (Each->IsInPlay())
		{
			UTEST_TRUE("Both halves are done splitting", Each->HasSplit());
			UTEST_TRUE("Both going back", Each->GetPlaneVelocity().X > 0.f);
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPBombBallTest, "IJPong.Ball.BombStunsThePaddleItBeat", IJPBallBehaviourTests::Flags)
bool FIJPBombBallTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test;
	AIJPArena* Arena = Test.GetArena();
	UIJPBallType* Bomb = NewObject<UIJPBallType>(GetTransientPackage());
	Bomb->StunOnGoal = 1.f;
	AIJPPaddle* Left = Arena->GetPaddle(EIJPSide::Left);

	UTEST_TRUE("Goal", IJPBallBehaviourTests::ScoreAgainst(Test, Arena, EIJPSide::Left, Bomb));
	UTEST_TRUE("Stunned", Left->IsStunned());
	const float Y = Left->GetPlanePosition().Y;
	Test.RunFor(0.5f, [Left] { Left->AddMoveInput(-1.f); });
	UTEST_EQUAL_TOLERANCE("Can't move", static_cast<float>(Left->GetPlanePosition().Y), static_cast<float>(Y), 0.01f);
	Test.RunFor(0.6f);
	UTEST_FALSE("Wears off", Left->IsStunned());
	Test.RunFor(0.3f, [Left] { Left->AddMoveInput(-1.f); });
	UTEST_TRUE("Moves again", Left->GetPlanePosition().Y < Y - 10.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPLeechBallTest, "IJPong.Ball.LeechHealsTheScorer", IJPBallBehaviourTests::Flags)
bool FIJPLeechBallTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test;
	AIJPTestGameMode* Mode = IJPBallBehaviourTests::GetMode(Test);
	AIJPArena* Arena = Mode->GetArena();
	UIJPMatchComponent* Match = Mode->GetMatch();
	UIJPBallType* Leech = NewObject<UIJPBallType>(GetTransientPackage());
	Leech->HealOnGoal = 1.f;
	Match->ApplyDamage(EIJPSide::Right, 2.f);

	UTEST_TRUE("Goal", IJPBallBehaviourTests::ScoreAgainst(Test, Arena, EIJPSide::Left, Leech));
	UTEST_EQUAL("It hurt the side it beat", Match->GetHealth(EIJPSide::Left), 4.f);
	UTEST_EQUAL("And healed the scorer", Match->GetHealth(EIJPSide::Right), 4.f);
	return true;
}

#endif
