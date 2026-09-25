// It's Just Pong

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Core/IJPRunGameMode.h"
#include "Core/IJPTypes.h"
#include "Engine/World.h"
#include "Gameplay/IJPMatchComponent.h"
#include "Gameplay/IJPMatchRules.h"
#include "Run/IJPActConfig.h"
#include "Run/IJPReward.h"
#include "Run/IJPRunMapView.h"
#include "Run/IJPRunSubsystem.h"
#include "Tests/IJPTestWorld.h"

namespace IJPShopTests
{
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter;

	UIJPReward_Modifier* MakeModifier(EIJPRunStat Stat, float Amount, int32 Price)
	{
		UIJPReward_Modifier* Reward = NewObject<UIJPReward_Modifier>(GetTransientPackage());
		Reward->Stat = Stat;
		Reward->Amount = Amount;
		Reward->Price = Price;
		return Reward;
	}

	/** Match, then a Shop, then Rest, then the Boss, in a straight line; rivals have 1 health. */
	UIJPActConfig* MakeShopAct(int32 LongerPrice, int32 TougherPrice)
	{
		UIJPActConfig* Act = NewObject<UIJPActConfig>(GetTransientPackage());
		Act->Rows = 4;
		Act->Lanes = 1;
		Act->Paths = 1;
		Act->MatchWeight = Act->EliteWeight = Act->RestWeight = 0.f;
		Act->ShopWeight = 1.f;
		Act->ShopRewards = { MakeModifier(EIJPRunStat::PaddleLength, 0.1f, LongerPrice), MakeModifier(EIJPRunStat::MaxHealth, 2.f, TougherPrice) };
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPShopBuyTest, "IJPong.Run.ShopSellsWhatYouCanAfford", IJPShopTests::Flags)
bool FIJPShopBuyTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test;
	UIJPRunSubsystem* Run = UIJPRunSubsystem::Get(Test.GetWorld());
	Run->StartRun(IJPShopTests::MakeShopAct(8, 15), 1, 5);

	Run->EnterNode(Run->GetReachableNodes()[0]);
	Run->CompleteNode(true);
	UTEST_EQUAL("Paid", Run->GetCoins(), 10);
	const int32 ShopNode = Run->GetReachableNodes()[0];
	UTEST_EQUAL("A shop next", Run->GetMap().Nodes[ShopNode].Type, EIJPNodeType::Shop);

	UTEST_TRUE("In", Run->EnterNode(ShopNode));
	UTEST_TRUE("Shopping", Run->IsInShop());
	UTEST_EQUAL("Both on the shelf", Run->GetShopStock().Num(), 2);
	UTEST_TRUE("The map waits", Run->GetReachableNodes().IsEmpty());

	// Buy what the coins cover; the rest stays out of reach.
	const int32 Cheap = Run->GetShopStock()[0]->Price == 8 ? 0 : 1;
	UTEST_TRUE("Bought", Run->BuyFromShop(Cheap));
	UTEST_EQUAL("Paid for", Run->GetCoins(), 2);
	UTEST_EQUAL_TOLERANCE("Given to the run", Run->GetLoadout().PaddleLengthBonus, 0.1f, KINDA_SMALL_NUMBER);
	UTEST_EQUAL("Off the shelf", Run->GetShopStock().Num(), 1);
	UTEST_FALSE("Too dear", Run->BuyFromShop(0));
	UTEST_EQUAL("Coins kept", Run->GetCoins(), 2);

	Run->LeaveShop();
	UTEST_FALSE("Left", Run->IsInShop());
	UTEST_EQUAL("On to the rest", Run->GetMap().Nodes[Run->GetReachableNodes()[0]].Type, EIJPNodeType::Rest);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPShopScreenTest, "IJPong.Run.ShopScreenBuysThenLeaves", IJPShopTests::Flags)
bool FIJPShopScreenTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test(FTransform::Identity, nullptr, AIJPRunGameMode::StaticClass());
	AIJPRunGameMode* Mode = Cast<AIJPRunGameMode>(Test.GetWorld()->GetAuthGameMode());
	UIJPRunSubsystem* Run = UIJPRunSubsystem::Get(Test.GetWorld());
	Mode->PostMatchDelay = 0.1f;
	Mode->StartNewRun(IJPShopTests::MakeShopAct(5, 5), 1, 5);

	// Win the first match for its coins.
	Mode->HandleUIConfirm();
	Mode->GetMatch()->ApplyDamage(EIJPSide::Right, 1.f);
	for (int32 i = 0; i < 60 && Mode->GetPhase() != EIJPRunPhase::Map; ++i)
	{
		Test.Step();
	}
	UTEST_EQUAL("Back on the map", Mode->GetPhase(), EIJPRunPhase::Map);
	UTEST_EQUAL("With coins", Run->GetCoins(), 10);

	// Into the shop: its cards, then LEAVE.
	UTEST_TRUE("Enter", Mode->HandleUIConfirm());
	UTEST_EQUAL("Shop screen", Mode->GetPhase(), EIJPRunPhase::Shop);
	UTEST_TRUE("Cards up", Mode->GetMapView()->IsShowingCards());
	Mode->HandleUIConfirm();
	UTEST_EQUAL("Bought one", Run->GetCoins(), 5);
	UTEST_EQUAL("Still shopping", Mode->GetPhase(), EIJPRunPhase::Shop);
	Mode->HandleUIConfirm();
	UTEST_EQUAL("And the other", Run->GetCoins(), 0);
	UTEST_TRUE("Shelf empty", Run->GetShopStock().IsEmpty());

	// Only LEAVE is left.
	Mode->HandleUIConfirm();
	UTEST_EQUAL("Back to the map", Mode->GetPhase(), EIJPRunPhase::Map);
	UTEST_FALSE("Done shopping", Run->IsInShop());
	return true;
}

#endif
