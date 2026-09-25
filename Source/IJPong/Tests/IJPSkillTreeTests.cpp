// It's Just Pong

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Abilities/IJPAbility_Smash.h"
#include "Abilities/IJPAbilityComponent.h"
#include "Core/IJPRunGameMode.h"
#include "Core/IJPTypes.h"
#include "Engine/World.h"
#include "Gameplay/IJPArena.h"
#include "Gameplay/IJPBall.h"
#include "Gameplay/IJPPaddle.h"
#include "Gameplay/IJPPaddleClass.h"
#include "Meta/IJPMetaSubsystem.h"
#include "Meta/IJPSkillTree.h"
#include "Run/IJPActConfig.h"
#include "Run/IJPRunMapView.h"
#include "Run/IJPRunSubsystem.h"
#include "Tests/IJPTestWorld.h"

namespace IJPSkillTreeTests
{
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter;

	FIJPSkillNode MakeNode(FName Id, FName Parent, int32 Points, int32 Tokens, EIJPTreeEffect Effect, float Amount, FName Upgrade = NAME_None)
	{
		FIJPSkillNode Node;
		Node.Id = Id;
		Node.Parent = Parent;
		Node.SkillPoints = Points;
		Node.BossTokens = Tokens;
		Node.Effect = Effect;
		Node.Amount = Amount;
		Node.UpgradeName = Upgrade;
		return Node;
	}

	/** Length (2 pts) -> Angle (1 pt), and a Power keystone (1 token) hanging from the root in branch 1. */
	UIJPSkillTree* MakeTree()
	{
		UIJPSkillTree* Tree = NewObject<UIJPSkillTree>(GetTransientPackage(), TEXT("TestTree"));
		Tree->Nodes.Add(MakeNode(TEXT("Length"), NAME_None, 2, 0, EIJPTreeEffect::PaddleLength, 0.2f));
		Tree->Nodes.Add(MakeNode(TEXT("Angle"), TEXT("Length"), 1, 0, EIJPTreeEffect::ReturnAngle, 10.f));
		FIJPSkillNode& Key = Tree->Nodes.Add_GetRef(MakeNode(TEXT("Power"), NAME_None, 0, 1, EIJPTreeEffect::SkillUpgrade, 1.f, TEXT("Power")));
		Key.Branch = 1;
		return Tree;
	}

	bool RunUntil(FIJPTestWorld& Test, float MaxSeconds, TFunctionRef<bool()> Condition)
	{
		const int32 Steps = FMath::CeilToInt(MaxSeconds / FIJPTestWorld::FixedStep);
		for (int32 i = 0; i < Steps && !Condition(); ++i)
		{
			Test.Step();
		}
		return Condition();
	}

	/** Angle from horizontal, in degrees. */
	double AngleDeg(const AIJPBall* Ball)
	{
		const FVector2D V = Ball->GetPlaneVelocity();
		return FMath::RadiansToDegrees(FMath::Atan2(FMath::Abs(V.Y), FMath::Abs(V.X)));
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPTreeBuyTest, "IJPong.Meta.TreeNodesNeedTheirParentAndThePrice", IJPSkillTreeTests::Flags)
bool FIJPTreeBuyTest::RunTest(const FString& Parameters)
{
	UIJPSkillTree* Tree = IJPSkillTreeTests::MakeTree();
	{
		FIJPTestWorld Test;
		UIJPMetaSubsystem* Meta = UIJPMetaSubsystem::Get(Test.GetWorld());
		Meta->ResetProgress();

		UTEST_FALSE("Can't afford it yet", Meta->CanBuy(Tree, 0));
		Meta->AddCurrency(3, 0);
		UTEST_FALSE("Child needs its parent first", Meta->CanBuy(Tree, 1));
		UTEST_TRUE("Buy the first node", Meta->Buy(Tree, 0));
		UTEST_EQUAL("Paid", Meta->GetSkillPoints(), 1);
		UTEST_FALSE("Can't buy twice", Meta->Buy(Tree, 0));
		UTEST_TRUE("Now the child", Meta->Buy(Tree, 1));
		UTEST_FALSE("Keystone costs a boss token", Meta->CanBuy(Tree, 2));
		Meta->AddCurrency(0, 1);
		UTEST_TRUE("With a token", Meta->Buy(Tree, 2));
	}

	// Owned nodes are saved with the currencies.
	FIJPTestWorld Later;
	UIJPMetaSubsystem* Meta = UIJPMetaSubsystem::Get(Later.GetWorld());
	UTEST_TRUE("Still owned after a restart", Meta->IsOwned(Tree, 0) && Meta->IsOwned(Tree, 1) && Meta->IsOwned(Tree, 2));
	const FIJPTreeBonuses Bonuses = Meta->GetBonuses(Tree);
	UTEST_EQUAL("Length bonus", Bonuses.PaddleLength, 0.2f);
	UTEST_EQUAL("Angle bonus", Bonuses.ReturnAngle, 10.f);
	UTEST_EQUAL("Skill upgrade", Bonuses.SkillUpgrades.FindRef(TEXT("Power")), 1.f);
	Meta->ResetProgress();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPTreeLevelTest, "IJPong.Meta.ALevelOpensWhenItsEraIsReached", IJPSkillTreeTests::Flags)
bool FIJPTreeLevelTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test;
	UIJPMetaSubsystem* Meta = UIJPMetaSubsystem::Get(Test.GetWorld());
	Meta->ResetProgress();
	UIJPSkillTree* Tree = IJPSkillTreeTests::MakeTree();
	FIJPSkillNode& Later = Tree->Nodes.Add_GetRef(IJPSkillTreeTests::MakeNode(TEXT("Later"), NAME_None, 1, 0, EIJPTreeEffect::PaddleSpeed, 0.1f));
	Later.Level = 1;
	const int32 LaterIndex = Tree->Nodes.Num() - 1;
	Meta->AddCurrency(5, 0);

	UTEST_TRUE("The first era's level is open", Meta->IsTreeLevelOpen(0));
	UTEST_FALSE("The second era's isn't yet", Meta->IsTreeLevelOpen(1));
	UTEST_FALSE("So its node can't be bought, even affordable", Meta->CanBuy(Tree, LaterIndex));
	UTEST_EQUAL("Two levels", Tree->GetNumLevels(), 2);

	Meta->UnlockErasUpTo(2);
	UTEST_TRUE("Reaching the second era opens it", Meta->IsTreeLevelOpen(1));
	UTEST_TRUE("Now buyable", Meta->Buy(Tree, LaterIndex));
	Meta->ResetProgress();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPTreeRunTest, "IJPong.Meta.TreeBonusesApplyToEveryRun", IJPSkillTreeTests::Flags)
bool FIJPTreeRunTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test(FTransform::Identity, nullptr, AIJPRunGameMode::StaticClass());
	AIJPRunGameMode* Mode = Cast<AIJPRunGameMode>(Test.GetWorld()->GetAuthGameMode());
	UIJPMetaSubsystem* Meta = UIJPMetaSubsystem::Get(Test.GetWorld());
	AIJPPaddle* Left = Mode->GetArena()->GetPaddle(EIJPSide::Left);
	Meta->ResetProgress();

	// The player's class, with a tree whose nodes are all owned.
	UIJPSkillTree* Tree = IJPSkillTreeTests::MakeTree();
	UIJPPaddleClass* PaddleClass = NewObject<UIJPPaddleClass>(GetTransientPackage());
	PaddleClass->ClassSkill = NewObject<UIJPAbility_Smash>(GetTransientPackage());
	PaddleClass->SkillTree = Tree;
	Left->SetPaddleClass(PaddleClass);
	const double BaseLength = Left->GetSize().Y;
	Meta->AddCurrency(3, 1);
	Meta->Buy(Tree, 0);
	Meta->Buy(Tree, 1);
	Meta->Buy(Tree, 2);

	UIJPActConfig* Act = NewObject<UIJPActConfig>(GetTransientPackage());
	Act->Rows = 3;
	Act->Lanes = 1;
	Act->Paths = 1;
	Mode->StartNewRun(Act, 1, 5);
	UTEST_EQUAL_TOLERANCE("Longer paddle from the tree", Left->GetSize().Y, BaseLength * 1.2, 0.01);
	UTEST_EQUAL("Steeper returns", Left->GetReturnAngleBonus(), 10.f);
	UTEST_EQUAL("Class skill upgraded", Left->GetAbilities()->GetAbility(EIJPAbilitySlot::ClassSkill)->GetUpgrade(TEXT("Power")), 1.f);
	Meta->ResetProgress();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPTreeScreenTest, "IJPong.Run.TreeScreenBetweenRuns", IJPSkillTreeTests::Flags)
bool FIJPTreeScreenTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test(FTransform::Identity, nullptr, AIJPRunGameMode::StaticClass());
	AIJPRunGameMode* Mode = Cast<AIJPRunGameMode>(Test.GetWorld()->GetAuthGameMode());
	UIJPMetaSubsystem* Meta = UIJPMetaSubsystem::Get(Test.GetWorld());
	UIJPRunSubsystem* Run = UIJPRunSubsystem::Get(Test.GetWorld());
	AIJPPaddle* Left = Mode->GetArena()->GetPaddle(EIJPSide::Left);
	Meta->ResetProgress();

	UIJPSkillTree* Tree = IJPSkillTreeTests::MakeTree();
	UIJPPaddleClass* PaddleClass = NewObject<UIJPPaddleClass>(GetTransientPackage());
	PaddleClass->SkillTree = Tree;
	Left->SetPaddleClass(PaddleClass);

	// Lose a run straight away (1 health, one goal) so the run ends.
	UIJPActConfig* Act = NewObject<UIJPActConfig>(GetTransientPackage());
	Act->Rows = 3;
	Act->Lanes = 1;
	Act->Paths = 1;
	Act->SkillPointsPerRow = 2;
	Mode->StartNewRun(Act, 1, 1);
	Mode->HandleUIConfirm();
	Test.RunFor(0.5f, [Left] { Left->AddMoveInput(1.f); });
	Mode->GetArena()->GetBall()->Serve(EIJPSide::Left, 0.f);
	UTEST_TRUE("Run over", IJPSkillTreeTests::RunUntil(Test, 3.f, [Mode] { return Mode->GetPhase() == EIJPRunPhase::Ended; }));
	UTEST_EQUAL("Earned 2 points", Meta->GetSkillPoints(), 2);

	UTEST_TRUE("Confirm opens the tree", Mode->HandleUIConfirm());
	UTEST_EQUAL("Tree screen", Mode->GetPhase(), EIJPRunPhase::Tree);
	UTEST_TRUE("Drawn", Mode->GetMapView()->IsShowingTree());
	UTEST_EQUAL("Pick starts on the first node", Mode->GetMapView()->GetSelectedTreeNode(), 0);
	Mode->HandleUIConfirm();
	UTEST_TRUE("Bought it", Meta->IsOwned(Tree, 0));
	UTEST_EQUAL("Spent", Meta->GetSkillPoints(), 0);

	for (int32 i = 0; i < 5; ++i)
	{
		Mode->HandleUIStep(1);
	}
	UTEST_EQUAL("START RUN is last", Mode->GetMapView()->GetSelectedTreeNode(), static_cast<int32>(INDEX_NONE));
	Mode->HandleUIConfirm();
	UTEST_EQUAL("New run on the map", Mode->GetPhase(), EIJPRunPhase::Map);
	UTEST_EQUAL("Running", Run->GetState(), EIJPRunState::Running);
	Meta->ResetProgress();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPSmashKeystoneTest, "IJPong.Ability.SmashKeystonesChangeTheSmash", IJPSkillTreeTests::Flags)
bool FIJPSmashKeystoneTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test;
	AIJPArena* Arena = Test.GetArena();
	AIJPBall* Ball = Arena->GetBall();
	AIJPPaddle* Left = Arena->GetPaddle(EIJPSide::Left);
	Arena->GetPaddle(EIJPSide::Right)->GetController()->UnPossess();
	UIJPAbilityComponent* Abilities = Left->GetAbilities();
	Abilities->Equip(EIJPAbilitySlot::ClassSkill, NewObject<UIJPAbility_Smash>(GetTransientPackage()));
	UIJPAbility* Smash = Abilities->GetAbility(EIJPAbilitySlot::ClassSkill);
	Smash->SetUpgrades({ { TEXT("ExtraHits"), 1.f }, { TEXT("Curve"), 1.f }, { TEXT("Split"), 1.f } });
	Test.RunFor(1.1f);

	Abilities->TryActivate(EIJPAbilitySlot::ClassSkill);
	Ball->Serve(EIJPSide::Left, 0.f);
	UTEST_TRUE("Returned", IJPSkillTreeTests::RunUntil(Test, 2.f, [Ball] { return Ball->GetRallyHits() >= 1; }));
	Test.Step();
	UTEST_TRUE("Double Smash: still armed for another return", Smash->IsArmed());
	UTEST_TRUE("Spin Smash: the return curves", Ball->IsCurving());
	UTEST_EQUAL("Shatter Smash: it split in two", Arena->GetNumBallsInPlay(), 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPReturnAngleTest, "IJPong.Ball.ReturnAngleBonusAllowsSteeperReturns", IJPSkillTreeTests::Flags)
bool FIJPReturnAngleTest::RunTest(const FString& Parameters)
{
	// A ball arriving near the paddle's top edge leaves near its steepest angle: steeper with the bonus.
	double Angles[2];
	for (int32 Pass = 0; Pass < 2; ++Pass)
	{
		FIJPTestWorld Test;
		AIJPArena* Arena = Test.GetArena();
		AIJPBall* Ball = Arena->GetBall();
		Arena->GetPaddle(EIJPSide::Left)->SetReturnAngleBonus(Pass == 0 ? 0.f : 15.f);
		Test.RunFor(1.1f);
		Ball->Serve(EIJPSide::Left, 6.43f); // meets the centred left paddle about 40 units up
		UTEST_TRUE("Returned", IJPSkillTreeTests::RunUntil(Test, 2.f, [Ball] { return Ball->GetRallyHits() >= 1; }));
		Angles[Pass] = IJPSkillTreeTests::AngleDeg(Ball);
	}
	UTEST_TRUE("Normal edge return stays within 60 degrees", Angles[0] <= 60.01);
	UTEST_TRUE("With +15, well past 60", Angles[1] > 63.0);
	return true;
}

#endif
