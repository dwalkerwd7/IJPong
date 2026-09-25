// It's Just Pong

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Core/IJPTestGameMode.h"
#include "Core/IJPTypes.h"
#include "Engine/World.h"
#include "Gameplay/IJPArena.h"
#include "Gameplay/IJPBall.h"
#include "Gameplay/IJPPaddle.h"
#include "Gameplay/IJPSevenSegmentComponent.h"
#include "Presentation/IJPCRTComponent.h"
#include "Tests/IJPTestWorld.h"

namespace IJPPresentationTests
{
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter;

	bool RunUntil(FIJPTestWorld& Test, float MaxSeconds, TFunctionRef<bool()> Condition)
	{
		const int32 Steps = FMath::CeilToInt(MaxSeconds / FIJPTestWorld::FixedStep);
		for (int32 i = 0; i < Steps && !Condition(); ++i)
		{
			Test.Step();
		}
		return Condition();
	}

	/** Get the left paddle out of the way and put a flat serve into its goal. Returns when the goal is scored. */
	bool ScoreOnLeft(FIJPTestWorld& Test)
	{
		AIJPArena* Arena = Test.GetArena();
		AIJPPaddle* Left = Arena->GetPaddle(EIJPSide::Left);
		AIJPBall* Ball = Arena->GetBall();
		Test.RunFor(0.5f, [Left] { Left->AddMoveInput(1.f); });
		Ball->Serve(EIJPSide::Left, 0.f);
		return RunUntil(Test, 2.f, [Ball] { return !Ball->IsInPlay(); });
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPServeBlinkTest, "IJPong.Presentation.BallBlinksBeforeServe", IJPPresentationTests::Flags)
bool FIJPServeBlinkTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test;
	AIJPBall* Ball = Test.GetArena()->GetBall();

	// The test mode schedules the first serve at start: the ball waits at the centre, blinking.
	UTEST_TRUE("Blinking while waiting to be served", Ball->IsBlinking());
	UTEST_TRUE("Starts hidden", Ball->IsHidden());
	UTEST_TRUE("At the centre", Ball->GetPlanePosition().IsNearlyZero());

	bool bSeenShown = false;
	bool bSeenHidden = false;
	IJPPresentationTests::RunUntil(Test, 2.f, [&]
	{
		if (!Ball->IsInPlay())
		{
			(Ball->IsHidden() ? bSeenHidden : bSeenShown) = true;
		}
		return Ball->IsInPlay();
	});
	UTEST_TRUE("Blinked on and off during the serve delay", bSeenShown && bSeenHidden);
	UTEST_TRUE("Served", Ball->IsInPlay());
	UTEST_FALSE("Stopped blinking once served", Ball->IsBlinking());
	UTEST_FALSE("Visible in play", Ball->IsHidden());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPScoreFlashTest, "IJPong.Presentation.HurtSideHealthFlashes", IJPPresentationTests::Flags)
bool FIJPScoreFlashTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test;
	AIJPArena* Arena = Test.GetArena();
	UIJPSevenSegmentComponent* Hurt = Arena->GetScoreDisplay(EIJPSide::Left);
	UIJPSevenSegmentComponent* Other = Arena->GetScoreDisplay(EIJPSide::Right);
	UTEST_EQUAL("Starts at full health (the configured rules' 5)", Hurt->GetValue(), 5);

	UTEST_TRUE("Goal", IJPPresentationTests::ScoreOnLeft(Test));
	UTEST_EQUAL("The side that conceded shows its health drop", Hurt->GetValue(), 4);
	UTEST_TRUE("Its digits flash", Hurt->IsFlashing());
	UTEST_FALSE("Starts with the digits off", Hurt->IsVisible());
	UTEST_FALSE("The other side's digits don't flash", Other->IsFlashing());

	UTEST_TRUE("Flash finishes", IJPPresentationTests::RunUntil(Test, 2.f, [Hurt] { return !Hurt->IsFlashing(); }));
	UTEST_TRUE("Ends with the digits shown", Hurt->IsVisible());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPGoalPulseTest, "IJPong.Presentation.GoalPulsesTheScreen", IJPPresentationTests::Flags)
bool FIJPGoalPulseTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test;
	UIJPCRTComponent* CRT = Test.GetArena()->GetCRT();
	UTEST_EQUAL("No pulse before the goal", CRT->GetFlash(), 0.f);

	UTEST_TRUE("Goal", IJPPresentationTests::ScoreOnLeft(Test));
	UTEST_TRUE("Screen pulses on the goal", CRT->GetFlash() > 0.5f);

	Test.RunFor(0.5f);
	UTEST_EQUAL("Pulse has faded out", CRT->GetFlash(), 0.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPPaddleFlickerTest, "IJPong.Presentation.PaddleFlickersOnHit", IJPPresentationTests::Flags)
bool FIJPPaddleFlickerTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test;
	AIJPArena* Arena = Test.GetArena();
	AIJPBall* Ball = Arena->GetBall();
	AIJPPaddle* Left = Arena->GetPaddle(EIJPSide::Left);
	AIJPPaddle* Right = Arena->GetPaddle(EIJPSide::Right);

	// Flat serve at the idle, centred left paddle.
	Ball->Serve(EIJPSide::Left, 0.f);
	UTEST_TRUE("Paddle hit", IJPPresentationTests::RunUntil(Test, 2.f, [Ball] { return Ball->GetRallyHits() == 1; }));
	UTEST_FALSE("Hit paddle blinks off", Left->IsVisualShown());
	UTEST_TRUE("The other paddle doesn't", Right->IsVisualShown());

	Test.RunFor(0.15f);
	UTEST_TRUE("Back on straight away", Left->IsVisualShown());
	return true;
}

#endif
