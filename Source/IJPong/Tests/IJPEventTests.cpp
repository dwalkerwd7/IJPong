// It's Just Pong

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Core/IJPRunGameMode.h"
#include "Core/IJPTypes.h"
#include "Engine/World.h"
#include "Gameplay/IJPMatchComponent.h"
#include "Gameplay/IJPMatchRules.h"
#include "Run/IJPActConfig.h"
#include "Run/IJPEvent.h"
#include "Run/IJPRunMapView.h"
#include "Run/IJPRunSubsystem.h"
#include "Tests/IJPTestWorld.h"

namespace IJPEventTests
{
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter;

	/** PAY: 5 coins for +3 max health. RISK: a sure loss of 10 health. DEAR: costs 50 coins. LEAVE: nothing. */
	UIJPEvent* MakeEvent()
	{
		UIJPEvent* Event = NewObject<UIJPEvent>(GetTransientPackage());
		Event->Title = FText::FromString(TEXT("TEST"));
		Event->Text = FText::FromString(TEXT("SOMETHING HAPPENS."));

		FIJPEventOption& Pay = Event->Options.AddDefaulted_GetRef();
		Pay.Label = FText::FromString(TEXT("PAY"));
		Pay.Success.Coins = -5;
		Pay.Success.MaxHealth = 3.f;
		Pay.Success.Text = FText::FromString(TEXT("PAID."));

		FIJPEventOption& Risk = Event->Options.AddDefaulted_GetRef();
		Risk.Label = FText::FromString(TEXT("RISK"));
		Risk.Chance = 0.f;
		Risk.Success.Coins = 100;
		Risk.Failure.Health = -10.f;

		FIJPEventOption& Dear = Event->Options.AddDefaulted_GetRef();
		Dear.Label = FText::FromString(TEXT("DEAR"));
		Dear.Success.Coins = -50;

		FIJPEventOption& Leave = Event->Options.AddDefaulted_GetRef();
		Leave.Label = FText::FromString(TEXT("LEAVE"));
		return Event;
	}

	/** Match, then two Events, then Rest, then the Boss, in a straight line; rivals have 1 health. */
	UIJPActConfig* MakeEventAct()
	{
		UIJPActConfig* Act = NewObject<UIJPActConfig>(GetTransientPackage());
		Act->Rows = 5;
		Act->Lanes = 1;
		Act->Paths = 1;
		Act->MatchWeight = Act->EliteWeight = Act->RestWeight = Act->ShopWeight = 0.f;
		Act->EventWeight = 1.f;
		Act->Events = { MakeEvent() };
		UIJPMatchRules* OneHealth = NewObject<UIJPMatchRules>(Act);
		OneHealth->StartingHealth = 1.f;
		OneHealth->ServeDelay = 0.2f;
		for (FIJPEncounter* Encounter : { &Act->Match, &Act->Elite, &Act->Boss })
		{
			Encounter->Rules = OneHealth;
			Encounter->Coins = 10;
		}
		return Act;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPEventChoiceTest, "IJPong.Run.EventOptionsTradeAndNeverKill", IJPEventTests::Flags)
bool FIJPEventChoiceTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test;
	UIJPRunSubsystem* Run = UIJPRunSubsystem::Get(Test.GetWorld());
	Run->StartRun(IJPEventTests::MakeEventAct(), 1, 5);
	Run->EnterNode(Run->GetReachableNodes()[0]);
	Run->CompleteNode(true);
	UTEST_EQUAL("Paid for the match", Run->GetCoins(), 10);

	const int32 EventNode = Run->GetReachableNodes()[0];
	UTEST_EQUAL("An event next", Run->GetMap().Nodes[EventNode].Type, EIJPNodeType::Event);
	UTEST_TRUE("In", Run->EnterNode(EventNode));
	UTEST_TRUE("At the event", Run->IsInEvent());
	UTEST_TRUE("The map waits", Run->GetReachableNodes().IsEmpty());

	UTEST_FALSE("Can't afford DEAR", Run->ChooseEventOption(2));
	UTEST_TRUE("Still deciding", Run->IsInEvent());
	UTEST_TRUE("PAY", Run->ChooseEventOption(0));
	UTEST_EQUAL("Paid", Run->GetCoins(), 5);
	UTEST_EQUAL_TOLERANCE("Tougher", Run->GetMaxHealth(), 8.f, KINDA_SMALL_NUMBER);
	UTEST_TRUE("Says what happened", Run->GetEventResult().StartsWith(TEXT("PAID.")));
	UTEST_FALSE("Done", Run->IsInEvent());

	// The second event: a sure loss that would kill leaves 1 health instead.
	Run->EnterNode(Run->GetReachableNodes()[0]);
	UTEST_TRUE("Another event", Run->IsInEvent());
	UTEST_TRUE("RISK", Run->ChooseEventOption(1));
	UTEST_FALSE("It went badly", Run->WasEventSuccess());
	UTEST_EQUAL_TOLERANCE("Left on 1", Run->GetHealth(), 1.f, KINDA_SMALL_NUMBER);
	UTEST_EQUAL("Still running", Run->GetState(), EIJPRunState::Running);
	UTEST_EQUAL("On to the rest", Run->GetMap().Nodes[Run->GetReachableNodes()[0]].Type, EIJPNodeType::Rest);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPEventScreenTest, "IJPong.Run.EventScreenTypesTextThenShowsTheResult", IJPEventTests::Flags)
bool FIJPEventScreenTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test(FTransform::Identity, nullptr, AIJPRunGameMode::StaticClass());
	AIJPRunGameMode* Mode = Cast<AIJPRunGameMode>(Test.GetWorld()->GetAuthGameMode());
	UIJPRunSubsystem* Run = UIJPRunSubsystem::Get(Test.GetWorld());
	Mode->PostMatchDelay = 0.1f;
	Mode->StartNewRun(IJPEventTests::MakeEventAct(), 1, 5);

	// Win the first match (its pick is skipped: no reward pool).
	Mode->HandleUIConfirm();
	Mode->GetMatch()->ApplyDamage(EIJPSide::Right, 1.f);
	for (int32 i = 0; i < 60 && Mode->GetPhase() != EIJPRunPhase::Map; ++i)
	{
		Test.Step();
	}
	UTEST_EQUAL("Back on the map", Mode->GetPhase(), EIJPRunPhase::Map);

	UTEST_TRUE("Enter", Mode->HandleUIConfirm());
	UTEST_EQUAL("Event screen", Mode->GetPhase(), EIJPRunPhase::Event);
	AIJPRunMapView* View = Mode->GetMapView();
	UTEST_TRUE("Options as cards", View->IsShowingCards());
	UTEST_TRUE("Text starts empty", View->GetBodyShown().IsEmpty());
	Test.RunFor(0.2f);
	UTEST_TRUE("Typing", !View->GetBodyShown().IsEmpty() && View->GetBodyShown() != TEXT("SOMETHING HAPPENS."));
	Test.RunFor(1.f);
	UTEST_EQUAL("Typed out", View->GetBodyShown(), FString(TEXT("SOMETHING HAPPENS.")));

	// PAY is the first card.
	Mode->HandleUIConfirm();
	UTEST_EQUAL("The result", Mode->GetPhase(), EIJPRunPhase::EventResult);
	UTEST_EQUAL("Paid", Run->GetCoins(), 5);
	Mode->HandleUIConfirm();
	UTEST_EQUAL("Back to the map", Mode->GetPhase(), EIJPRunPhase::Map);
	return true;
}

#endif
