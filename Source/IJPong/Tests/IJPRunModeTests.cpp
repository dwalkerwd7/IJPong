// It's Just Pong

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Core/IJPRunGameMode.h"
#include "Core/IJPTypes.h"
#include "Engine/World.h"
#include "Gameplay/IJPArena.h"
#include "Gameplay/IJPBall.h"
#include "Gameplay/IJPMatchComponent.h"
#include "Gameplay/IJPMatchRules.h"
#include "Gameplay/IJPPaddle.h"
#include "Run/IJPActConfig.h"
#include "Run/IJPRunMapView.h"
#include "Run/IJPRunSubsystem.h"
#include "Tests/IJPTestWorld.h"

namespace IJPRunModeTests
{
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter;

	AIJPRunGameMode* GetMode(const FIJPTestWorld& Test)
	{
		return Cast<AIJPRunGameMode>(Test.GetWorld()->GetAuthGameMode());
	}

	/** A straight act (Match, Rest, Boss) where every fight is first to 1. */
	UIJPActConfig* MakeStraightAct()
	{
		UIJPActConfig* Act = NewObject<UIJPActConfig>(GetTransientPackage());
		Act->Rows = 3;
		Act->Lanes = 1;
		Act->Paths = 1;
		UIJPMatchRules* OnePoint = NewObject<UIJPMatchRules>(Act);
		OnePoint->WinTarget = 1;
		OnePoint->ServeDelay = 0.2f;
		for (FIJPEncounter* Encounter : { &Act->Match, &Act->Elite, &Act->Boss })
		{
			Encounter->Rules = OnePoint;
		}
		return Act;
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

	/** Hold the player's paddle at the top and put the ball straight into their goal. */
	bool Concede(FIJPTestWorld& Test, AIJPArena* Arena)
	{
		AIJPPaddle* Left = Arena->GetPaddle(EIJPSide::Left);
		const auto HoldUp = [Left] { Left->AddMoveInput(1.f); };
		Test.RunFor(0.5f, HoldUp);
		AIJPBall* Ball = Arena->GetBall();
		Ball->Serve(EIJPSide::Left, 0.f);
		return RunUntil(Test, 3.f, [Ball] { return !Ball->IsInPlay(); }, HoldUp);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPRunNodeTest, "IJPong.Run.FightNodePlaysThenReturnsToTheMap", IJPRunModeTests::Flags)
bool FIJPRunNodeTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test(FTransform::Identity, nullptr, AIJPRunGameMode::StaticClass());
	AIJPRunGameMode* Mode = IJPRunModeTests::GetMode(Test);
	UTEST_NOT_NULL("Run game mode", Mode);
	UIJPRunSubsystem* Run = UIJPRunSubsystem::Get(Test.GetWorld());
	Mode->PostMatchDelay = 0.5f;
	Mode->StartNewRun(IJPRunModeTests::MakeStraightAct(), 1, 5);
	UTEST_EQUAL("Starts on the map", Mode->GetPhase(), EIJPRunPhase::Map);
	UTEST_NOT_EQUAL("Something to pick", Mode->GetMapView()->GetSelectedNode(), static_cast<int32>(INDEX_NONE));

	UTEST_TRUE("Confirm enters the node", Mode->HandleUIConfirm());
	UTEST_EQUAL("Playing", Mode->GetPhase(), EIJPRunPhase::Playing);
	UTEST_TRUE("In a fight", Run->IsInNode());
	UTEST_FALSE("Confirm is the class skill again mid-match", Mode->HandleUIConfirm());

	// Lose the first-to-1 match by conceding.
	UTEST_TRUE("Conceded", IJPRunModeTests::Concede(Test, Mode->GetArena()));
	UTEST_EQUAL("It cost health", Run->GetHealth(), 4);
	UTEST_TRUE("Match over", Mode->GetMatch()->IsOver());

	UTEST_TRUE("Back to the map", IJPRunModeTests::RunUntil(Test, 2.f, [Mode] { return Mode->GetPhase() == EIJPRunPhase::Map; }));
	UTEST_EQUAL("A loss pays nothing", Run->GetCoins(), 0);
	UTEST_EQUAL("Run carries on", Run->GetState(), EIJPRunState::Running);
	UTEST_EQUAL("Next pick is the rest", Run->GetMap().Nodes[Mode->GetMapView()->GetSelectedNode()].Type, EIJPNodeType::Rest);

	// Rest heals on the spot and stays on the map.
	UTEST_TRUE("Rest", Mode->HandleUIConfirm());
	UTEST_EQUAL("Still on the map", Mode->GetPhase(), EIJPRunPhase::Map);
	UTEST_EQUAL("Healed back to full", Run->GetHealth(), 5);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPRunOverTest, "IJPong.Run.RunEndsWhenHealthRunsOut", IJPRunModeTests::Flags)
bool FIJPRunOverTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test(FTransform::Identity, nullptr, AIJPRunGameMode::StaticClass());
	AIJPRunGameMode* Mode = IJPRunModeTests::GetMode(Test);
	UIJPRunSubsystem* Run = UIJPRunSubsystem::Get(Test.GetWorld());
	UIJPActConfig* Act = IJPRunModeTests::MakeStraightAct();
	Act->Match.Rules->WinTarget = 5; // so it's the health that ends things, not the match
	Mode->StartNewRun(Act, 1, 1);

	Mode->HandleUIConfirm();
	UTEST_TRUE("Conceded", IJPRunModeTests::Concede(Test, Mode->GetArena()));
	UTEST_EQUAL("Out of health", Run->GetState(), EIJPRunState::Lost);
	UTEST_EQUAL("Run over", Mode->GetPhase(), EIJPRunPhase::Ended);
	Test.RunFor(1.f);
	UTEST_FALSE("The match stopped: no serve", Mode->GetArena()->GetBall()->IsInPlay());

	UTEST_TRUE("Confirm starts a new run", Mode->HandleUIConfirm());
	UTEST_EQUAL("Running again", Run->GetState(), EIJPRunState::Running);
	UTEST_EQUAL("On the map", Mode->GetPhase(), EIJPRunPhase::Map);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPRunPickTest, "IJPong.Run.MapPickStepsAcrossReachableNodes", IJPRunModeTests::Flags)
bool FIJPRunPickTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test(FTransform::Identity, nullptr, AIJPRunGameMode::StaticClass());
	AIJPRunGameMode* Mode = IJPRunModeTests::GetMode(Test);
	UIJPRunSubsystem* Run = UIJPRunSubsystem::Get(Test.GetWorld());
	UIJPActConfig* Act = IJPRunModeTests::MakeStraightAct();
	Act->Rows = 6;
	Act->Lanes = 4;
	Act->Paths = 3;
	Mode->StartNewRun(Act, 3, 5);

	const TArray<int32> Start = Run->GetReachableNodes();
	UTEST_TRUE("A choice", Start.Num() >= 2);
	AIJPRunMapView* Map = Mode->GetMapView();
	UTEST_EQUAL("Starts on the leftmost", Map->GetSelectedNode(), Start[0]);
	UTEST_TRUE("Right steps", Mode->HandleUIStep(1));
	UTEST_EQUAL("To the next one right", Map->GetSelectedNode(), Start[1]);
	Mode->HandleUIStep(-1);
	Mode->HandleUIStep(-1);
	UTEST_EQUAL("Stops at the left end", Map->GetSelectedNode(), Start[0]);

	Mode->HandleUIStep(1);
	Mode->HandleUIConfirm();
	UTEST_EQUAL("Entered the picked node", Run->GetCurrentNode(), Start[1]);
	UTEST_FALSE("Steps don't move anything mid-match", Mode->HandleUIStep(1));
	return true;
}

#endif
