// It's Just Pong

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Core/IJPRunGameMode.h"
#include "Engine/World.h"
#include "Era/IJPEra.h"
#include "Era/IJPEraSubsystem.h"
#include "Gameplay/IJPMatchRules.h"
#include "Meta/IJPMetaSubsystem.h"
#include "Meta/IJPSkillTree.h"
#include "Abilities/IJPItem_Shield.h"
#include "Run/IJPActConfig.h"
#include "Run/IJPReward.h"
#include "Run/IJPRunSubsystem.h"
#include "Run/IJPRunMapView.h"
#include "Tests/IJPTestWorld.h"

namespace IJPMapIconTests
{
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter;

	/** Match, Rest, Boss in a straight line. */
	UIJPActConfig* MakeStraightAct()
	{
		UIJPActConfig* Act = NewObject<UIJPActConfig>(GetTransientPackage());
		Act->Rows = 3;
		Act->Lanes = 1;
		Act->Paths = 1;
		UIJPMatchRules* Rules = NewObject<UIJPMatchRules>(Act);
		for (FIJPEncounter* Encounter : { &Act->Match, &Act->Elite, &Act->Boss })
		{
			Encounter->Rules = Rules;
		}
		return Act;
	}

	void UseSpriteEra(UWorld* World)
	{
		UIJPEra* Era = NewObject<UIJPEra>(GetTransientPackage());
		Era->bShowSprites = true;
		UIJPEraSubsystem::Get(World)->SetEra(Era);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPMapIconTest, "IJPong.Run.MapNodesWearIconsInSpriteEras", IJPMapIconTests::Flags)
bool FIJPMapIconTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test(FTransform::Identity, nullptr, AIJPRunGameMode::StaticClass());
	AIJPRunGameMode* Mode = Cast<AIJPRunGameMode>(Test.GetWorld()->GetAuthGameMode());
	UIJPMetaSubsystem::Get(Test.GetWorld())->ResetProgress();
	Mode->StartNewRun(IJPMapIconTests::MakeStraightAct(), 1, 5);
	AIJPRunMapView* View = Mode->GetMapView();
	UTEST_EQUAL("1972: no icons", View->GetShownIconCount(), 0);
	UTEST_EQUAL("1972: a letter per node", View->GetShownGlyphCount(), 3);

	IJPMapIconTests::UseSpriteEra(Test.GetWorld());
	View->Refresh();
	UTEST_EQUAL("Sprite era: an icon per node", View->GetShownIconCount(), 3);
	UTEST_EQUAL("And no letters", View->GetShownGlyphCount(), 0);

	// The tree too: its root plus each node.
	UIJPSkillTree* Tree = NewObject<UIJPSkillTree>(GetTransientPackage(), TEXT("IconTree"));
	FIJPSkillNode& Stat = Tree->Nodes.AddDefaulted_GetRef();
	Stat.Id = TEXT("Stat");
	Stat.SkillPoints = 1;
	FIJPSkillNode& Key = Tree->Nodes.AddDefaulted_GetRef();
	Key.Id = TEXT("Key");
	Key.SkillPoints = 0;
	Key.BossTokens = 1;
	Key.Branch = 1;
	View->ShowTree(Tree, true);
	UTEST_EQUAL("Tree: root + two nodes", View->GetShownIconCount(), 3);

	UIJPMetaSubsystem::Get(Test.GetWorld())->ResetProgress();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPLoadoutLineTest, "IJPong.Run.TheMapAndCardsShowWhatYouCarry", IJPMapIconTests::Flags)
bool FIJPLoadoutLineTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test(FTransform::Identity, nullptr, AIJPRunGameMode::StaticClass());
	AIJPRunGameMode* Mode = Cast<AIJPRunGameMode>(Test.GetWorld()->GetAuthGameMode());
	UIJPMetaSubsystem* Meta = UIJPMetaSubsystem::Get(Test.GetWorld());
	Meta->ResetProgress();
	Mode->StartNewRun(IJPMapIconTests::MakeStraightAct(), 1, 5);
	AIJPRunMapView* View = Mode->GetMapView();
	UTEST_TRUE("On the map", View->IsShowingLoadout());
	UTEST_TRUE("Empty-handed", View->GetLoadoutLine().Contains(TEXT("ITEM NONE")));
	UTEST_FALSE("No spell slot yet, so no spell", View->GetLoadoutLine().Contains(TEXT("SPELL")));

	// Pick up a shield: the line names it.
	UIJPItem_Shield* Shield = NewObject<UIJPItem_Shield>(GetTransientPackage());
	Shield->DisplayName = FText::FromString(TEXT("Shield"));
	UIJPReward_Item* Reward = NewObject<UIJPReward_Item>(GetTransientPackage());
	Reward->Item = Shield;
	Reward->Grant(*UIJPRunSubsystem::Get(Test.GetWorld()));
	View->Refresh();
	UTEST_TRUE("Names the item", View->GetLoadoutLine().Contains(TEXT("ITEM SHIELD")));

	// The card screens show it too.
	View->ShowCards(TEXT("SHOP"), {}, 0);
	UTEST_TRUE("On the cards", View->IsShowingLoadout());
	Meta->ResetProgress();
	return true;
}

#endif
