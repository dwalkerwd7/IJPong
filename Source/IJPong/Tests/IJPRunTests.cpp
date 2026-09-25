// It's Just Pong

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Engine/World.h"
#include "Run/IJPActConfig.h"
#include "Run/IJPRunMap.h"
#include "Run/IJPRunSubsystem.h"
#include "Tests/IJPTestWorld.h"

namespace IJPRunTests
{
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter;

	UIJPActConfig* MakeAct(int32 Rows = 7, int32 Lanes = 4, int32 Paths = 3)
	{
		UIJPActConfig* Act = NewObject<UIJPActConfig>(GetTransientPackage());
		Act->Rows = Rows;
		Act->Lanes = Lanes;
		Act->Paths = Paths;
		return Act;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPMapShapeTest, "IJPong.Run.MapFollowsTheActsRules", IJPRunTests::Flags)
bool FIJPMapShapeTest::RunTest(const FString& Parameters)
{
	const UIJPActConfig* Act = IJPRunTests::MakeAct();

	// Many seeds, so the rules hold for maps in general, not one lucky layout.
	for (int32 Seed = 0; Seed < 50; ++Seed)
	{
		FRandomStream Random(Seed);
		const FIJPRunMap Map = FIJPRunMap::Generate(*Act, Random);
		const FString Tag = FString::Printf(TEXT("seed %d: "), Seed);

		UTEST_EQUAL(Tag + TEXT("rows"), Map.Rows, 7);
		UTEST_TRUE(Tag + TEXT("a choice at the top"), Map.GetStartNodes().Num() >= 2);

		TArray<bool> Reached;
		Reached.Init(false, Map.Nodes.Num());
		for (const int32 Start : Map.GetStartNodes())
		{
			Reached[Start] = true;
		}
		for (int32 Row = 0; Row < Map.Rows; ++Row)
		{
			for (int32 i = 0; i < Map.Nodes.Num(); ++i)
			{
				const FIJPMapNode& Node = Map.Nodes[i];
				if (Node.Row != Row)
				{
					continue;
				}
				if (Row == 0) { UTEST_EQUAL(Tag + TEXT("top row is Match"), Node.Type, EIJPNodeType::Match); }
				if (Row == Map.Rows - 2) { UTEST_EQUAL(Tag + TEXT("rest before the boss"), Node.Type, EIJPNodeType::Rest); }
				if (Row == Map.Rows - 1) { UTEST_EQUAL(Tag + TEXT("boss alone at the bottom"), i, Map.BossIndex); }
				if (Node.Type == EIJPNodeType::Elite) { UTEST_TRUE(Tag + TEXT("no early elites"), Row >= Act->EliteFromRow); }
				UTEST_TRUE(Tag + TEXT("never shop/event yet"), Node.Type != EIJPNodeType::Shop && Node.Type != EIJPNodeType::Event);
				UTEST_TRUE(Tag + TEXT("everything is reachable"), Reached[i]);
				UTEST_TRUE(Tag + TEXT("everything leads on"), i == Map.BossIndex || !Node.Next.IsEmpty());

				for (const int32 NextIndex : Node.Next)
				{
					const FIJPMapNode& Next = Map.Nodes[NextIndex];
					Reached[NextIndex] = true;
					UTEST_EQUAL(Tag + TEXT("edges go one row down"), Next.Row, Row + 1);
					UTEST_TRUE(Tag + TEXT("no rest right after a rest"), !(Node.Type == EIJPNodeType::Rest && Next.Type == EIJPNodeType::Rest));
					if (NextIndex != Map.BossIndex)
					{
						UTEST_TRUE(Tag + TEXT("at most one lane sideways"), FMath::Abs(Next.Lane - Node.Lane) <= 1);
					}
				}
			}
		}

		// No two paths between the same rows cross.
		for (const FIJPMapNode& A : Map.Nodes)
		{
			for (const FIJPMapNode& B : Map.Nodes)
			{
				if (A.Row != B.Row || A.Row >= Map.Rows - 2)
				{
					continue;
				}
				for (const int32 AN : A.Next)
				{
					for (const int32 BN : B.Next)
					{
						const bool bCross = A.Lane < B.Lane && Map.Nodes[AN].Lane > Map.Nodes[BN].Lane;
						UTEST_FALSE(Tag + TEXT("paths never cross"), bCross);
					}
				}
			}
		}
	}

	// Same seed, same map.
	FRandomStream First(1234), Second(1234);
	const FIJPRunMap MapA = FIJPRunMap::Generate(*Act, First);
	const FIJPRunMap MapB = FIJPRunMap::Generate(*Act, Second);
	UTEST_EQUAL("Reproducible: node count", MapA.Nodes.Num(), MapB.Nodes.Num());
	for (int32 i = 0; i < MapA.Nodes.Num(); ++i)
	{
		UTEST_TRUE("Reproducible: node", MapA.Nodes[i].Type == MapB.Nodes[i].Type && MapA.Nodes[i].Lane == MapB.Nodes[i].Lane && MapA.Nodes[i].Next == MapB.Nodes[i].Next);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPRunMovesTest, "IJPong.Run.MovingDownTheMapPaysAndHeals", IJPRunTests::Flags)
bool FIJPRunMovesTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test;
	UIJPRunSubsystem* Run = UIJPRunSubsystem::Get(Test.GetWorld());
	UTEST_NOT_NULL("Run subsystem", Run);

	// A single straight path: Match, Rest, Boss.
	UIJPActConfig* Act = IJPRunTests::MakeAct(3, 1, 1);
	Act->Match.Coins = 10;
	Act->Boss.Coins = 50;
	Act->RestHeal = 2;
	Run->StartRun(Act, 7, 5);
	UTEST_EQUAL("Running", Run->GetState(), EIJPRunState::Running);
	UTEST_EQUAL("Full health", Run->GetHealth(), 5);

	const TArray<int32> Start = Run->GetReachableNodes();
	UTEST_EQUAL("One way in", Start.Num(), 1);
	UTEST_FALSE("Can't skip to the boss", Run->EnterNode(Run->GetMap().BossIndex));

	UTEST_TRUE("Enter the match", Run->EnterNode(Start[0]));
	UTEST_TRUE("In a fight", Run->IsInNode());
	UTEST_TRUE("Nowhere to go mid-fight", Run->GetReachableNodes().IsEmpty());
	Run->LoseHealth(3);
	Run->CompleteNode(true);
	UTEST_EQUAL("Paid for the win", Run->GetCoins(), 10);
	UTEST_EQUAL("Goals against cost health", Run->GetHealth(), 2);

	const int32 RestNode = Run->GetReachableNodes()[0];
	UTEST_EQUAL("Then the rest", Run->GetMap().Nodes[RestNode].Type, EIJPNodeType::Rest);
	UTEST_TRUE("Rest", Run->EnterNode(RestNode));
	UTEST_FALSE("Resting is instant", Run->IsInNode());
	UTEST_EQUAL("Healed", Run->GetHealth(), 4);

	UTEST_TRUE("On to the boss", Run->EnterNode(Run->GetMap().BossIndex));
	Run->CompleteNode(false);
	UTEST_EQUAL("A loss pays nothing", Run->GetCoins(), 10);
	UTEST_EQUAL("And doesn't end the run by itself", Run->GetState(), EIJPRunState::Running);

	// Health running out does.
	Run->StartRun(Act, 7, 2);
	Run->EnterNode(Run->GetReachableNodes()[0]);
	Run->LoseHealth(2);
	UTEST_EQUAL("Out of health: lost", Run->GetState(), EIJPRunState::Lost);
	UTEST_TRUE("Nowhere to go", Run->GetReachableNodes().IsEmpty());

	// Beating the boss wins.
	Run->StartRun(Act, 7, 5);
	Run->EnterNode(Run->GetReachableNodes()[0]);
	Run->CompleteNode(true);
	Run->EnterNode(Run->GetReachableNodes()[0]);
	Run->EnterNode(Run->GetReachableNodes()[0]);
	Run->CompleteNode(true);
	UTEST_EQUAL("Boss beaten: won", Run->GetState(), EIJPRunState::Won);
	UTEST_EQUAL("Boss paid too", Run->GetCoins(), 60);
	return true;
}

#endif
