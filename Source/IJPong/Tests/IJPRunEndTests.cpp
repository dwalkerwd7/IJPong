// It's Just Pong

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Audio/IJPToneSynthComponent.h"
#include "Core/IJPRunGameMode.h"
#include "Core/IJPTypes.h"
#include "Engine/World.h"
#include "Gameplay/IJPArena.h"
#include "Gameplay/IJPMatchComponent.h"
#include "Gameplay/IJPMatchRules.h"
#include "Meta/IJPMetaSubsystem.h"
#include "Run/IJPActConfig.h"
#include "Run/IJPRunMapView.h"
#include "Run/IJPRunSubsystem.h"
#include "Tests/IJPTestWorld.h"

namespace IJPRunEndTests
{
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter;

	/** Match, Rest, Boss in a straight line; rivals have RivalHealth. */
	UIJPActConfig* MakeStraightAct(float RivalHealth)
	{
		UIJPActConfig* Act = NewObject<UIJPActConfig>(GetTransientPackage());
		Act->Rows = 3;
		Act->Lanes = 1;
		Act->Paths = 1;
		UIJPMatchRules* Rules = NewObject<UIJPMatchRules>(Act);
		Rules->StartingHealth = RivalHealth;
		Rules->ServeDelay = 0.2f;
		for (FIJPEncounter* Encounter : { &Act->Match, &Act->Elite, &Act->Boss })
		{
			Encounter->Rules = Rules;
		}
		return Act;
	}

	bool WaitFor(FIJPTestWorld& Test, AIJPRunGameMode* Mode, EIJPRunPhase Phase)
	{
		for (int32 i = 0; i < 240 && Mode->GetPhase() != Phase; ++i)
		{
			Test.Step();
		}
		return Mode->GetPhase() == Phase;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPRunLostScreenTest, "IJPong.Run.LosingIsDeadpan", IJPRunEndTests::Flags)
bool FIJPRunLostScreenTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test(FTransform::Identity, nullptr, AIJPRunGameMode::StaticClass());
	AIJPRunGameMode* Mode = Cast<AIJPRunGameMode>(Test.GetWorld()->GetAuthGameMode());
	UIJPMetaSubsystem::Get(Test.GetWorld())->ResetProgress();
	Mode->StartNewRun(IJPRunEndTests::MakeStraightAct(5.f), 1, 1);
	Mode->HandleUIConfirm();
	Mode->GetMatch()->ApplyDamage(EIJPSide::Left, 1.f);
	UTEST_EQUAL("Run over", Mode->GetPhase(), EIJPRunPhase::Ended);

	AIJPRunMapView* View = Mode->GetMapView();
	UIJPToneSynthComponent* Tones = Mode->GetArena()->GetTones();
	UTEST_TRUE("The end screen", View->IsShowingRunEnd() && !View->IsWinScreen());
	const int32 TonesBefore = Tones->GetToneCount();
	Test.Step();
	UTEST_TRUE("Typing, not all at once", View->GetEndTitle().Len() < 5);
	Test.RunFor(3.5f);
	UTEST_EQUAL("It's just Pong", View->GetEndTitle(), FString(TEXT("IT'S JUST PONG...")));
	UTEST_EQUAL("A beep per letter", Tones->GetToneCount() - TonesBefore, 11);

	UIJPMetaSubsystem::Get(Test.GetWorld())->ResetProgress();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPRunWonScreenTest, "IJPong.Run.WinningIsAParty", IJPRunEndTests::Flags)
bool FIJPRunWonScreenTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test(FTransform::Identity, nullptr, AIJPRunGameMode::StaticClass());
	AIJPRunGameMode* Mode = Cast<AIJPRunGameMode>(Test.GetWorld()->GetAuthGameMode());
	UIJPMetaSubsystem::Get(Test.GetWorld())->ResetProgress();
	Mode->PostMatchDelay = 0.1f;
	Mode->StartNewRun(IJPRunEndTests::MakeStraightAct(1.f), 1, 5);

	// The match, the rest, the boss.
	Mode->HandleUIConfirm();
	Mode->GetMatch()->ApplyDamage(EIJPSide::Right, 1.f);
	UTEST_TRUE("Map", IJPRunEndTests::WaitFor(Test, Mode, EIJPRunPhase::Map));
	Mode->HandleUIConfirm();
	Mode->HandleUIConfirm();
	Mode->GetMatch()->ApplyDamage(EIJPSide::Right, 1.f);
	UTEST_TRUE("Won", IJPRunEndTests::WaitFor(Test, Mode, EIJPRunPhase::Ended));

	AIJPRunMapView* View = Mode->GetMapView();
	UIJPToneSynthComponent* Tones = Mode->GetArena()->GetTones();
	UTEST_TRUE("The party", View->IsShowingRunEnd() && View->IsWinScreen());
	UTEST_EQUAL("Congratulations", View->GetEndTitle(), FString(TEXT("CONGRATULATIONS!")));
	const int32 TonesBefore = Tones->GetToneCount();
	Test.RunFor(2.5f);
	UTEST_TRUE("A fanfare, then pops", Tones->GetToneCount() - TonesBefore >= 7);
	UTEST_TRUE("Fireworks", View->GetSparkCount() > 0);

	// Moving on clears it.
	Mode->HandleUIConfirm();
	UTEST_FALSE("Gone", View->IsShowingRunEnd());
	UIJPMetaSubsystem::Get(Test.GetWorld())->ResetProgress();
	return true;
}

#endif
