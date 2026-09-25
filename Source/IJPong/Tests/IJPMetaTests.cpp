// It's Just Pong

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Engine/World.h"
#include "Meta/IJPMetaSubsystem.h"
#include "Run/IJPActConfig.h"
#include "Run/IJPRunSubsystem.h"
#include "Tests/IJPTestWorld.h"

namespace IJPMetaTests
{
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter;

	/** Match, Rest, Boss in a straight line. */
	UIJPActConfig* MakeStraightAct()
	{
		UIJPActConfig* Act = NewObject<UIJPActConfig>(GetTransientPackage());
		Act->Rows = 3;
		Act->Lanes = 1;
		Act->Paths = 1;
		Act->SkillPointsPerRow = 2;
		Act->BossTokens = 1;
		return Act;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPMetaEarnTest, "IJPong.Meta.RunsPayDepthPointsAndBossesPayTokens", IJPMetaTests::Flags)
bool FIJPMetaEarnTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test;
	UIJPMetaSubsystem* Meta = UIJPMetaSubsystem::Get(Test.GetWorld());
	UIJPRunSubsystem* Run = UIJPRunSubsystem::Get(Test.GetWorld());
	UTEST_NOT_NULL("Meta subsystem", Meta);
	Meta->ResetProgress();
	const UIJPActConfig* Act = IJPMetaTests::MakeStraightAct();

	// Lost in the first row: depth 1.
	Run->StartRun(Act, 1, 1);
	Run->EnterNode(Run->GetReachableNodes()[0]);
	UTEST_EQUAL("Nothing paid mid-run", Meta->GetSkillPoints(), 0);
	Run->LoseHealth(1);
	UTEST_EQUAL("A loss still pays by depth", Run->GetEarnedSkillPoints(), 2);
	UTEST_EQUAL("No boss, no tokens", Run->GetEarnedBossTokens(), 0);
	UTEST_EQUAL("Banked", Meta->GetSkillPoints(), 2);

	// All the way down and beating the boss: depth 3, plus a token.
	Run->StartRun(Act, 1, 5);
	Run->EnterNode(Run->GetReachableNodes()[0]);
	Run->CompleteNode(true);
	Run->EnterNode(Run->GetReachableNodes()[0]);
	Run->EnterNode(Run->GetReachableNodes()[0]);
	Run->CompleteNode(true);
	UTEST_EQUAL("Won", Run->GetState(), EIJPRunState::Won);
	UTEST_EQUAL("Depth pays more", Run->GetEarnedSkillPoints(), 6);
	UTEST_EQUAL("The boss pays a token", Run->GetEarnedBossTokens(), 1);
	UTEST_EQUAL("Points add up across runs", Meta->GetSkillPoints(), 8);
	UTEST_EQUAL("Tokens banked", Meta->GetBossTokens(), 1);

	Meta->ResetProgress();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPMetaSaveTest, "IJPong.Meta.ProgressSurvivesARestart", IJPMetaTests::Flags)
bool FIJPMetaSaveTest::RunTest(const FString& Parameters)
{
	{
		FIJPTestWorld First;
		UIJPMetaSubsystem* Meta = UIJPMetaSubsystem::Get(First.GetWorld());
		Meta->ResetProgress();
		Meta->AddCurrency(5, 2);
	}

	// A new game instance loads what the last one saved.
	FIJPTestWorld Second;
	UIJPMetaSubsystem* Meta = UIJPMetaSubsystem::Get(Second.GetWorld());
	UTEST_EQUAL("Skill points kept", Meta->GetSkillPoints(), 5);
	UTEST_EQUAL("Boss tokens kept", Meta->GetBossTokens(), 2);

	Meta->ResetProgress();
	UTEST_EQUAL("Reset wipes it", Meta->GetSkillPoints(), 0);
	return true;
}

#endif
