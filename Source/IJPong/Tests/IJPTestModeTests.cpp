// It's Just Pong

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "AI/IJPPaddleAIController.h"
#include "Core/IJPTestGameMode.h"
#include "Core/IJPTypes.h"
#include "Engine/World.h"
#include "Gameplay/IJPArena.h"
#include "Gameplay/IJPBall.h"
#include "Gameplay/IJPPaddle.h"
#include "Tests/IJPTestWorld.h"

namespace IJPTestModeTests
{
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter;

	AIJPTestGameMode* GetMode(const FIJPTestWorld& Test)
	{
		return Cast<AIJPTestGameMode>(Test.GetWorld()->GetAuthGameMode());
	}

	bool RunUntil(FIJPTestWorld& Test, float MaxSeconds, TFunctionRef<bool()> Condition)
	{
		const int32 Steps = FMath::CeilToInt(MaxSeconds / FIJPTestWorld::FixedStep);
		for (int32 i = 0; i < Steps && !Condition(); ++i)
		{
			Test.Step();
		}
		return Condition();
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPTestModeResetTest, "IJPong.TestMode.ResetScoreZeroesAndReserves", IJPTestModeTests::Flags)
bool FIJPTestModeResetTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test;
	AIJPTestGameMode* Mode = IJPTestModeTests::GetMode(Test);
	UTEST_NOT_NULL("Test game mode", Mode);
	AIJPBall* Ball = Mode->GetBall();
	AIJPPaddle* Left = Mode->GetArena()->GetPaddle(EIJPSide::Left);

	// Concede one on the left: paddle out of the way, straight serve at its goal.
	Test.RunFor(0.5f, [Left] { Left->AddMoveInput(1.f); });
	Ball->Serve(EIJPSide::Left, 0.f);
	UTEST_TRUE("Goal", IJPTestModeTests::RunUntil(Test, 2.f, [Ball] { return !Ball->IsInPlay(); }));
	UTEST_EQUAL("Right scored", Mode->GetScore(EIJPSide::Right), 1);

	// Reset mid-rally: scores clear, the ball is taken out of play, and a fresh serve follows.
	Test.RunFor(1.2f);
	UTEST_TRUE("Next rally under way", Ball->IsInPlay());
	Mode->ResetScore();
	UTEST_EQUAL("Left score cleared", Mode->GetScore(EIJPSide::Left), 0);
	UTEST_EQUAL("Right score cleared", Mode->GetScore(EIJPSide::Right), 0);
	UTEST_FALSE("Rally abandoned", Ball->IsInPlay());
	UTEST_TRUE("Serves again after the delay", IJPTestModeTests::RunUntil(Test, 2.f, [Ball] { return Ball->IsInPlay(); }));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPTestModeServeNowTest, "IJPong.TestMode.ServeNowServesImmediately", IJPTestModeTests::Flags)
bool FIJPTestModeServeNowTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test;
	AIJPTestGameMode* Mode = IJPTestModeTests::GetMode(Test);
	UTEST_NOT_NULL("Test game mode", Mode);
	AIJPBall* Ball = Mode->GetBall();

	UTEST_FALSE("Waiting for the first serve", Ball->IsInPlay());
	Mode->ServeNow();
	UTEST_TRUE("In play without waiting", Ball->IsInPlay());

	// Mid-rally it restarts from the centre with a fresh rally.
	Test.RunFor(0.5f);
	Mode->ServeNow();
	UTEST_TRUE("Back at the centre", Ball->GetPlanePosition().IsNearlyZero());
	UTEST_EQUAL("Fresh rally", Ball->GetRallyHits(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPTestModeAIvsAITest, "IJPong.TestMode.PlayerSideAIToggle", IJPTestModeTests::Flags)
bool FIJPTestModeAIvsAITest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test;
	AIJPTestGameMode* Mode = IJPTestModeTests::GetMode(Test);
	UTEST_NOT_NULL("Test game mode", Mode);
	AIJPBall* Ball = Mode->GetBall();
	AIJPPaddle* Left = Mode->GetArena()->GetPaddle(AIJPTestGameMode::PlayerSide);

	UTEST_FALSE("Starts with the player side human", Mode->IsPlayerSideAI());
	Mode->SetPlayerSideAI(true);
	UTEST_TRUE("Player side AI on", Mode->IsPlayerSideAI());
	UTEST_TRUE("Player paddle driven by an AI", Cast<AIJPPaddleAIController>(Left->GetController()) != nullptr);
	Mode->SetPlayerSideAI(true);
	UTEST_TRUE("Turning it on twice is harmless", Mode->IsPlayerSideAI());

	// The level plays itself: the (formerly player) left paddle returns shots.
	bool bLeftReturned = false;
	IJPTestModeTests::RunUntil(Test, 30.f, [&]
	{
		// A left return is a paddle hit that leaves the ball heading right.
		bLeftReturned |= Ball->GetRallyHits() > 0 && Ball->GetPlaneVelocity().X > 0.f && Ball->GetPlanePosition().X < 0.f;
		return bLeftReturned;
	});
	UTEST_TRUE("Left AI returned a shot", bLeftReturned);

	Mode->SetPlayerSideAI(false);
	UTEST_FALSE("Player side AI off", Mode->IsPlayerSideAI());
	// No local player exists in a test world, so the paddle is simply left free for one.
	UTEST_NULL("AI released the paddle", Left->GetController());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPTestModeSkillTest, "IJPong.TestMode.AdjustOpponentSkillLive", IJPTestModeTests::Flags)
bool FIJPTestModeSkillTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test;
	AIJPTestGameMode* Mode = IJPTestModeTests::GetMode(Test);
	UTEST_NOT_NULL("Test game mode", Mode);
	AIJPArena* Arena = Mode->GetArena();
	AIJPPaddleAIController* AI = Cast<AIJPPaddleAIController>(Arena->GetPaddle(EIJPSide::Right)->GetController());
	UTEST_NOT_NULL("AI", AI);
	const float Start = Arena->GetOpponentSkill();

	Mode->AdjustOpponentSkill(0.2f);
	UTEST_EQUAL_TOLERANCE("Arena skill raised", Arena->GetOpponentSkill(), Start + 0.2f, KINDA_SMALL_NUMBER);
	UTEST_EQUAL_TOLERANCE("Applied to the AI live", AI->GetSkill(), Arena->GetOpponentSkill(), KINDA_SMALL_NUMBER);

	Mode->AdjustOpponentSkill(5.f);
	UTEST_EQUAL("Clamped at 1", Arena->GetOpponentSkill(), 1.f);
	Mode->AdjustOpponentSkill(-5.f);
	UTEST_EQUAL("Clamped at 0", Arena->GetOpponentSkill(), 0.f);
	UTEST_EQUAL("AI follows", AI->GetSkill(), 0.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPTestModeOverlayTest, "IJPong.TestMode.OverlayShowsToolState", IJPTestModeTests::Flags)
bool FIJPTestModeOverlayTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test;
	AIJPTestGameMode* Mode = IJPTestModeTests::GetMode(Test);
	UTEST_NOT_NULL("Test game mode", Mode);

	auto Overlay = [Mode]
	{
		TArray<FString> Lines;
		Mode->GetDebugLines(Lines);
		return FString::Join(Lines, TEXT(" | "));
	};

	UTEST_TRUE("Shows the opponent skill", Overlay().Contains(TEXT("Opponent skill: 0.50")));
	UTEST_TRUE("Shows who drives your paddle", Overlay().Contains(TEXT("Your paddle: you")));
	UTEST_TRUE("Lists the keys", Overlay().Contains(TEXT(". (period) hide this")));

	// It reflects changes as they happen.
	Mode->AdjustOpponentSkill(0.2f);
	Mode->SetPlayerSideAI(true);
	UTEST_TRUE("Skill updates", Overlay().Contains(TEXT("Opponent skill: 0.70")));
	UTEST_TRUE("AI vs AI shows", Overlay().Contains(TEXT("Your paddle: AI")));
	return true;
}

#endif
