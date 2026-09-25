// It's Just Pong

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Core/IJPRunGameMode.h"
#include "Core/IJPTypes.h"
#include "Engine/World.h"
#include "Era/IJPEra.h"
#include "Era/IJPEraSubsystem.h"
#include "Gameplay/IJPMatchComponent.h"
#include "Gameplay/IJPMatchRules.h"
#include "Meta/IJPMetaSubsystem.h"
#include "Run/IJPActConfig.h"
#include "Run/IJPRunSubsystem.h"
#include "Tests/IJPTestWorld.h"

namespace IJPClimbTests
{
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter;

	/** Match, Rest, Boss in a straight line; rivals have 1 health. */
	UIJPActConfig* MakeStraightAct()
	{
		UIJPActConfig* Act = NewObject<UIJPActConfig>(GetTransientPackage());
		Act->Rows = 3;
		Act->Lanes = 1;
		Act->Paths = 1;
		UIJPMatchRules* OneHealth = NewObject<UIJPMatchRules>(Act);
		OneHealth->StartingHealth = 1.f;
		OneHealth->ServeDelay = 0.2f;
		for (FIJPEncounter* Encounter : { &Act->Match, &Act->Elite, &Act->Boss })
		{
			Encounter->Rules = OneHealth;
		}
		return Act;
	}

	UIJPEra* MakeEra(const TCHAR* Name)
	{
		UIJPEra* Era = NewObject<UIJPEra>(GetTransientPackage(), Name);
		Era->DisplayName = FText::FromString(Name);
		return Era;
	}

	/** Clear the act the run is on: the match, the rest, the boss. */
	void ClearAct(UIJPRunSubsystem* Run)
	{
		Run->EnterNode(Run->GetReachableNodes()[0]);
		Run->CompleteNode(true);
		Run->EnterNode(Run->GetReachableNodes()[0]);
		Run->EnterNode(Run->GetReachableNodes()[0]);
		Run->CompleteNode(true);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPClimbStagesTest, "IJPong.Run.ClimbGoesActByActThenUnlocksTheNextEra", IJPClimbTests::Flags)
bool FIJPClimbStagesTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test;
	UIJPRunSubsystem* Run = UIJPRunSubsystem::Get(Test.GetWorld());
	UIJPMetaSubsystem* Meta = UIJPMetaSubsystem::Get(Test.GetWorld());
	Meta->ResetProgress();
	UIJPEra* First = IJPClimbTests::MakeEra(TEXT("First"));
	UIJPEra* Second = IJPClimbTests::MakeEra(TEXT("Second"));
	Run->StartRun({ { IJPClimbTests::MakeStraightAct(), First }, { IJPClimbTests::MakeStraightAct(), Second } }, 1, 5, 2);
	UTEST_EQUAL("Two acts", Run->GetNumStages(), 2);
	UTEST_TRUE("In the first era", Run->GetStageEra() == First);

	IJPClimbTests::ClearAct(Run);
	UTEST_EQUAL("The boss led on, not to a win", Run->GetState(), EIJPRunState::Running);
	UTEST_EQUAL("Second act", Run->GetStageIndex(), 1);
	UTEST_TRUE("In the second era", Run->GetStageEra() == Second);
	UTEST_FALSE("A fresh map to start", Run->GetReachableNodes().IsEmpty());
	UTEST_EQUAL("Depth keeps counting", Run->GetDepthReached(), 3);
	UTEST_EQUAL("Not unlocked yet", Meta->GetErasUnlocked(), 1);

	IJPClimbTests::ClearAct(Run);
	UTEST_EQUAL("The last boss wins it", Run->GetState(), EIJPRunState::Won);
	UTEST_TRUE("And unlocks the next era", Run->DidUnlockEra());
	UTEST_EQUAL("Saved", Meta->GetErasUnlocked(), 2);

	Meta->ResetProgress();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPEraChangeTest, "IJPong.Run.EraChangesBetweenActsOfDifferentEras", IJPClimbTests::Flags)
bool FIJPEraChangeTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test(FTransform::Identity, nullptr, AIJPRunGameMode::StaticClass());
	AIJPRunGameMode* Mode = Cast<AIJPRunGameMode>(Test.GetWorld()->GetAuthGameMode());
	UIJPEraSubsystem* Eras = UIJPEraSubsystem::Get(Test.GetWorld());
	UIJPMetaSubsystem* Meta = UIJPMetaSubsystem::Get(Test.GetWorld());
	Meta->ResetProgress();
	Mode->PostMatchDelay = 0.1f;
	Mode->EraChangeTime = 1.f;
	UIJPEra* First = IJPClimbTests::MakeEra(TEXT("FirstEra"));
	UIJPEra* Second = IJPClimbTests::MakeEra(TEXT("SecondEra"));
	Mode->StartClimb({ { IJPClimbTests::MakeStraightAct(), First }, { IJPClimbTests::MakeStraightAct(), Second } }, 1, 5);
	UTEST_TRUE("Starts in its first era", Eras->GetEra() == First);

	auto WaitFor = [&](EIJPRunPhase Phase)
	{
		for (int32 i = 0; i < 180 && Mode->GetPhase() != Phase; ++i)
		{
			Test.Step();
		}
		return Mode->GetPhase() == Phase;
	};
	auto Win = [&]()
	{
		Mode->HandleUIConfirm();
		Mode->GetMatch()->ApplyDamage(EIJPSide::Right, 1.f);
	};

	Win();
	UTEST_TRUE("Back to the map", WaitFor(EIJPRunPhase::Map));
	Mode->HandleUIConfirm(); // rest
	Win(); // the boss
	UTEST_TRUE("The era change", WaitFor(EIJPRunPhase::EraChange));
	UTEST_TRUE("Still the old era while the tube switches off", Eras->GetEra() == First);
	Mode->HandleUIConfirm();
	UTEST_EQUAL("Can't skip it the first time", Mode->GetPhase(), EIJPRunPhase::EraChange);
	UTEST_TRUE("Then the new map", WaitFor(EIJPRunPhase::Map));
	UTEST_TRUE("In the new era", Eras->GetEra() == Second);
	UTEST_TRUE("Remembered as seen", Meta->HasSeenEraCard(Second->GetName()));

	Eras->SetEraIndex(0);
	Meta->ResetProgress();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPBuildClimbTest, "IJPong.Run.NewRunsClimbEveryUnlockedEra", IJPClimbTests::Flags)
bool FIJPBuildClimbTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test(FTransform::Identity, nullptr, AIJPRunGameMode::StaticClass());
	AIJPRunGameMode* Mode = Cast<AIJPRunGameMode>(Test.GetWorld()->GetAuthGameMode());
	UIJPEraSubsystem* Eras = UIJPEraSubsystem::Get(Test.GetWorld());
	UIJPMetaSubsystem* Meta = UIJPMetaSubsystem::Get(Test.GetWorld());
	Meta->ResetProgress();
	UTEST_TRUE("At least two eras set up", Eras->GetNumEras() >= 2);
	const UIJPEra* Cabinet = Eras->GetEraAt(0);
	const UIJPEra* Arcade = Eras->GetEraAt(1);

	// Only the first era: all its acts, and winning unlocks the second.
	int32 UnlocksTo = 0;
	TArray<FIJPRunStage> Climb = Mode->BuildClimb(UnlocksTo);
	UTEST_EQUAL("The first era in full", Climb.Num(), Cabinet->Acts.Num());
	UTEST_EQUAL("Unlocks the second", UnlocksTo, 2);

	// Two eras: the first briefly, the newest in full.
	Meta->UnlockErasUpTo(2);
	Climb = Mode->BuildClimb(UnlocksTo);
	UTEST_EQUAL("Beaten era briefly, newest in full", Climb.Num(), Cabinet->ActsWhenBeaten + Arcade->Acts.Num());
	UTEST_TRUE("Starts in the first era", Climb[0].Era == Cabinet);
	UTEST_TRUE("Ends in the newest", Climb.Last().Era == Arcade);

	Meta->ResetProgress();
	return true;
}

#endif
