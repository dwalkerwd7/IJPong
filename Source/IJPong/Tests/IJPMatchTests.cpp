// It's Just Pong

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Core/IJPTestGameMode.h"
#include "Core/IJPTypes.h"
#include "Engine/World.h"
#include "Gameplay/IJPArena.h"
#include "Gameplay/IJPBall.h"
#include "Gameplay/IJPMatchComponent.h"
#include "Gameplay/IJPMatchRules.h"
#include "Gameplay/IJPPaddle.h"
#include "Gameplay/IJPSevenSegmentComponent.h"
#include "Tests/IJPTestWorld.h"

namespace IJPMatchTests
{
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter;

	AIJPTestGameMode* GetMode(const FIJPTestWorld& Test)
	{
		return Cast<AIJPTestGameMode>(Test.GetWorld()->GetAuthGameMode());
	}

	UIJPMatchRules* MakeRules(int32 WinTarget)
	{
		UIJPMatchRules* Rules = NewObject<UIJPMatchRules>();
		Rules->WinTarget = WinTarget;
		Rules->ServeDelay = 0.2f;
		return Rules;
	}

	/** Nobody defends: the AI lets go of the right paddle, and both paddles hide against the top wall. */
	void AbandonBothGoals(AIJPArena* Arena)
	{
		if (AController* AI = Arena->GetPaddle(EIJPSide::Right)->GetController())
		{
			AI->UnPossess();
		}
	}

	void HidePaddles(AIJPArena* Arena)
	{
		Arena->GetPaddle(EIJPSide::Left)->AddMoveInput(1.f);
		Arena->GetPaddle(EIJPSide::Right)->AddMoveInput(1.f);
	}

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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPMatchWinTest, "IJPong.Match.FirstToTargetWinsAndStops", IJPMatchTests::Flags)
bool FIJPMatchWinTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test;
	AIJPTestGameMode* Mode = IJPMatchTests::GetMode(Test);
	UTEST_NOT_NULL("Test game mode", Mode);
	AIJPArena* Arena = Mode->GetArena();
	AIJPBall* Ball = Arena->GetBall();
	UIJPMatchComponent* Match = Mode->GetMatch();

	Mode->RestartMatch(IJPMatchTests::MakeRules(2));
	IJPMatchTests::AbandonBothGoals(Arena);
	UTEST_TRUE("Match ends", IJPMatchTests::RunUntil(Test, 30.f, [Match] { return Match->IsOver(); }, [Arena] { IJPMatchTests::HidePaddles(Arena); }));

	const EIJPSide Winner = Match->GetWinner();
	const EIJPSide Loser = IJP::Opposite(Winner);
	UTEST_EQUAL("Winner reached the target", Match->GetScore(Winner), 2);
	UTEST_TRUE("Loser didn't", Match->GetScore(Loser) < 2);
	UTEST_EQUAL("Phase is match over", Match->GetPhase(), EIJPMatchPhase::MatchOver);
	UTEST_FALSE("Ball out of play", Ball->IsInPlay());
	UTEST_TRUE("Ball hidden", Ball->IsHidden());
	UTEST_FALSE("Ball isn't waiting to serve", Ball->IsBlinking());
	UTEST_TRUE("Winner's score blinks", Arena->GetScoreDisplay(Winner)->IsFlashing());
	UTEST_FALSE("Loser's score doesn't", Arena->GetScoreDisplay(Loser)->IsFlashing());

	// It waits: no more serves, and the winner keeps blinking.
	Test.RunFor(5.f, [Arena] { IJPMatchTests::HidePaddles(Arena); });
	UTEST_FALSE("Still no serve", Ball->IsInPlay());
	UTEST_EQUAL("Winner's score unchanged", Match->GetScore(Winner), 2);
	UTEST_TRUE("Still blinking", Arena->GetScoreDisplay(Winner)->IsFlashing());
	Match->ServeNow();
	UTEST_FALSE("The match's own serve does nothing once it's over", Ball->IsInPlay());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPMatchRestartTest, "IJPong.Match.ServeKeyStartsANewMatchAfterAWin", IJPMatchTests::Flags)
bool FIJPMatchRestartTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test;
	AIJPTestGameMode* Mode = IJPMatchTests::GetMode(Test);
	UTEST_NOT_NULL("Test game mode", Mode);
	AIJPArena* Arena = Mode->GetArena();
	AIJPBall* Ball = Arena->GetBall();
	UIJPMatchComponent* Match = Mode->GetMatch();

	Mode->RestartMatch(IJPMatchTests::MakeRules(1));
	IJPMatchTests::AbandonBothGoals(Arena);
	UTEST_TRUE("Match ends", IJPMatchTests::RunUntil(Test, 20.f, [Match] { return Match->IsOver(); }, [Arena] { IJPMatchTests::HidePaddles(Arena); }));
	const EIJPSide Winner = Match->GetWinner();

	// F once the match is over: a new match with the configured rules.
	Mode->ServeNow();
	UTEST_FALSE("No longer over", Match->IsOver());
	UTEST_EQUAL("Waiting to serve", Match->GetPhase(), EIJPMatchPhase::Serve);
	UTEST_EQUAL("Left score cleared", Match->GetScore(EIJPSide::Left), 0);
	UTEST_EQUAL("Right score cleared", Match->GetScore(EIJPSide::Right), 0);
	UTEST_EQUAL("Scoreboard cleared", Arena->GetScoreDisplay(Winner)->GetValue(), 0);
	UTEST_FALSE("Winner's blink stopped", Arena->GetScoreDisplay(Winner)->IsFlashing());
	UTEST_TRUE("Winner's score left showing", Arena->GetScoreDisplay(Winner)->IsVisible());
	UTEST_TRUE("Configured rules back in play", Match->GetRules().WinTarget == 5);
	UTEST_TRUE("Serves after the delay", IJPMatchTests::RunUntil(Test, 2.f, [Ball] { return Ball->IsInPlay(); }));
	UTEST_EQUAL("Rally under way", Match->GetPhase(), EIJPMatchPhase::Rally);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPMatchEndlessTest, "IJPong.Match.EndlessRulesNeverEnd", IJPMatchTests::Flags)
bool FIJPMatchEndlessTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test;
	AIJPTestGameMode* Mode = IJPMatchTests::GetMode(Test);
	UTEST_NOT_NULL("Test game mode", Mode);
	AIJPArena* Arena = Mode->GetArena();
	UIJPMatchComponent* Match = Mode->GetMatch();

	Mode->RestartMatch(IJPMatchTests::MakeRules(0));
	IJPMatchTests::AbandonBothGoals(Arena);
	auto Goals = [Match] { return Match->GetScore(EIJPSide::Left) + Match->GetScore(EIJPSide::Right); };
	bool bEverOver = false;
	UTEST_TRUE("Goals keep coming", IJPMatchTests::RunUntil(Test, 40.f, [&] { return Goals() >= 6; }, [&]
	{
		IJPMatchTests::HidePaddles(Arena);
		bEverOver |= Match->IsOver();
	}));
	UTEST_FALSE("Never over", bEverOver || Match->IsOver());
	return true;
}

#endif
