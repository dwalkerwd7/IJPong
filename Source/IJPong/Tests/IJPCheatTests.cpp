// It's Just Pong

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Core/IJPRunGameMode.h"
#include "Core/IJPRunPlayerController.h"
#include "Core/IJPTypes.h"
#include "Engine/World.h"
#include "Gameplay/IJPMatchComponent.h"
#include "Gameplay/IJPMatchRules.h"
#include "Meta/IJPMetaSubsystem.h"
#include "Run/IJPActConfig.h"
#include "Run/IJPRunSubsystem.h"
#include "Tests/IJPTestWorld.h"

namespace IJPCheatTests
{
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter;

	/** Three matches in a row, then Rest and the Boss; rivals have 3 health. */
	UIJPActConfig* MakeAct()
	{
		UIJPActConfig* Act = NewObject<UIJPActConfig>(GetTransientPackage());
		Act->Rows = 5;
		Act->Lanes = 1;
		Act->Paths = 1;
		Act->EliteWeight = Act->RestWeight = Act->ShopWeight = 0.f;
		UIJPMatchRules* Rules = NewObject<UIJPMatchRules>(Act);
		Rules->StartingHealth = 3.f;
		Rules->ServeDelay = 0.2f;
		for (FIJPEncounter* Encounter : { &Act->Match, &Act->Elite, &Act->Boss })
		{
			Encounter->Rules = Rules;
			Encounter->Coins = 10;
		}
		return Act;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPRunCheatsTest, "IJPong.Run.CheatsWinLoseHealPayAndUnlock", IJPCheatTests::Flags)
bool FIJPRunCheatsTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test(FTransform::Identity, nullptr, AIJPRunGameMode::StaticClass());
	AIJPRunGameMode* Mode = Cast<AIJPRunGameMode>(Test.GetWorld()->GetAuthGameMode());
	UIJPRunSubsystem* Run = UIJPRunSubsystem::Get(Test.GetWorld());
	UIJPMetaSubsystem* Meta = UIJPMetaSubsystem::Get(Test.GetWorld());
	UIJPMatchComponent* Match = Mode->GetMatch();
	Meta->ResetProgress();
	Mode->PostMatchDelay = 0.1f;
	Mode->StartNewRun(IJPCheatTests::MakeAct(), 1, 5);
	const auto WaitFor = [&](EIJPRunPhase Phase)
	{
		for (int32 i = 0; i < 60 && Mode->GetPhase() != Phase; ++i)
		{
			Test.Step();
		}
		return Mode->GetPhase() == Phase;
	};
	UTEST_TRUE("The run's own controller", Mode->PlayerControllerClass == AIJPRunPlayerController::StaticClass());

	// Win: straight back to the map, paid.
	Mode->HandleUIConfirm();
	UTEST_EQUAL("Playing", Mode->GetPhase(), EIJPRunPhase::Playing);
	Mode->CheatEndMatch(true);
	UTEST_EQUAL("The rival's out", Match->GetHealth(EIJPSide::Right), 0.f);
	UTEST_TRUE("Won and back", WaitFor(EIJPRunPhase::Map));
	UTEST_EQUAL("Paid", Run->GetCoins(), 10);

	// Lose a match with health to spare in the run: it costs all of the match's, which is the run's.
	Mode->HandleUIConfirm();
	Match->ApplyDamage(EIJPSide::Left, 2.f);
	Mode->CheatHeal();
	UTEST_EQUAL_TOLERANCE("Healed in the run", Run->GetHealth(), 5.f, 0.001f);
	UTEST_EQUAL_TOLERANCE("And in the match", Match->GetHealth(EIJPSide::Left), 5.f, 0.001f);
	Mode->CheatEndMatch(false);
	UTEST_EQUAL("Lost the run with it", Run->GetState(), EIJPRunState::Lost);

	// Coins, meta and eras.
	Mode->StartNewRun(IJPCheatTests::MakeAct(), 1, 5);
	Mode->CheatCoins(50);
	UTEST_EQUAL("Coins", Run->GetCoins(), 50);
	const int32 PointsBefore = Meta->GetSkillPoints();
	Mode->CheatMeta(10, 3);
	UTEST_EQUAL("Skill points", Meta->GetSkillPoints(), PointsBefore + 10);
	UTEST_EQUAL("Boss tokens", Meta->GetBossTokens(), 3);
	const int32 ErasBefore = Meta->GetErasUnlocked();
	Mode->CheatUnlockNextEra();
	UTEST_TRUE("An era more (if there is one)", Meta->GetErasUnlocked() >= ErasBefore);

	Meta->ResetProgress();
	return true;
}

#endif
