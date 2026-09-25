// It's Just Pong

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Abilities/IJPAbilityComponent.h"
#include "Abilities/IJPItem_Patch.h"
#include "Abilities/IJPItem_Shield.h"
#include "Core/IJPRunGameMode.h"
#include "Core/IJPTestGameMode.h"
#include "Core/IJPTypes.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "Gameplay/IJPArena.h"
#include "Gameplay/IJPBall.h"
#include "Gameplay/IJPMatchComponent.h"
#include "Gameplay/IJPMatchRules.h"
#include "Gameplay/IJPPaddle.h"
#include "Run/IJPActConfig.h"
#include "Run/IJPReward.h"
#include "Run/IJPRunSubsystem.h"
#include "Tests/IJPTestWorld.h"

namespace IJPItemTests
{
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter;

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

	UIJPItem_Patch* MakePatch(float Heal)
	{
		UIJPItem_Patch* Patch = NewObject<UIJPItem_Patch>(GetTransientPackage());
		Patch->Heal = Heal;
		return Patch;
	}

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
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPPatchTest, "IJPong.Item.PatchHealsOnceThenItsGone", IJPItemTests::Flags)
bool FIJPPatchTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test;
	AIJPTestGameMode* Mode = Cast<AIJPTestGameMode>(Test.GetWorld()->GetAuthGameMode());
	UIJPMatchComponent* Match = Mode->GetMatch();
	UIJPAbilityComponent* Abilities = Mode->GetArena()->GetPaddle(EIJPSide::Left)->GetAbilities();
	Abilities->Equip(EIJPAbilitySlot::Item, IJPItemTests::MakePatch(2.f));
	UTEST_TRUE("Carrying it", Abilities->HasItem());

	Match->ApplyDamage(EIJPSide::Left, 4.f);
	UTEST_TRUE("Used", Abilities->TryActivate(EIJPAbilitySlot::Item));
	UTEST_EQUAL("Healed", Match->GetHealth(EIJPSide::Left), 3.f);
	UTEST_FALSE("Gone", Abilities->HasItem());
	UTEST_FALSE("One use", Abilities->TryActivate(EIJPAbilitySlot::Item));

	// Never past the maximum.
	Abilities->Equip(EIJPAbilitySlot::Item, IJPItemTests::MakePatch(10.f));
	Abilities->TryActivate(EIJPAbilitySlot::Item);
	UTEST_EQUAL("Capped at full", Match->GetHealth(EIJPSide::Left), 5.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPShieldTest, "IJPong.Item.ShieldBlocksTheNextGoal", IJPItemTests::Flags)
bool FIJPShieldTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test;
	AIJPTestGameMode* Mode = Cast<AIJPTestGameMode>(Test.GetWorld()->GetAuthGameMode());
	AIJPArena* Arena = Mode->GetArena();
	UIJPMatchComponent* Match = Mode->GetMatch();
	AIJPPaddle* Player = Arena->GetPaddle(EIJPSide::Left);
	Arena->GetPaddle(EIJPSide::Right)->GetController()->UnPossess();
	const auto HoldUp = [Player] { Player->AddMoveInput(1.f); };
	Test.RunFor(0.5f, HoldUp);

	Player->GetAbilities()->Equip(EIJPAbilitySlot::Item, NewObject<UIJPItem_Shield>(GetTransientPackage()));
	UTEST_TRUE("Used", Player->GetAbilities()->TryActivate(EIJPAbilitySlot::Item));
	UTEST_TRUE("Shield up", Arena->IsBarrierUp(EIJPSide::Left));

	// A ball straight at the open goal bounces off it, and the shield drops.
	AIJPBall* Ball = Arena->GetBall();
	Ball->Serve(EIJPSide::Left, 0.f);
	UTEST_TRUE("Blocked", IJPItemTests::RunUntil(Test, 2.f, [Ball] { return Ball->GetPlaneVelocity().X > 0.f; }, HoldUp));
	UTEST_EQUAL("No damage", Match->GetHealth(EIJPSide::Left), 5.f);
	UTEST_FALSE("Shield spent", Arena->IsBarrierUp(EIJPSide::Left));

	// The next one goes in.
	Ball->Serve(EIJPSide::Left, 0.f);
	UTEST_TRUE("Goal", IJPItemTests::RunUntil(Test, 2.f, [Ball] { return !Ball->IsInPlay(); }, HoldUp));
	UTEST_EQUAL("That one hurt", Match->GetHealth(EIJPSide::Left), 4.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPRunItemTest, "IJPong.Item.RunCarriesOneItemFromPicksAndBosses", IJPItemTests::Flags)
bool FIJPRunItemTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test(FTransform::Identity, nullptr, AIJPRunGameMode::StaticClass());
	AIJPRunGameMode* Mode = Cast<AIJPRunGameMode>(Test.GetWorld()->GetAuthGameMode());
	UIJPRunSubsystem* Run = UIJPRunSubsystem::Get(Test.GetWorld());
	UIJPActConfig* Act = IJPItemTests::MakeStraightAct();
	UIJPItem_Patch* Patch = IJPItemTests::MakePatch(2.f);
	Act->BossItems = { Patch };
	Mode->StartNewRun(Act, 1, 5);

	// A reward card gives the item (replacing any held).
	UIJPReward_Item* Card = NewObject<UIJPReward_Item>(GetTransientPackage());
	Card->Item = Patch;
	Card->Grant(*Run);
	UTEST_TRUE("Carried", Run->GetLoadout().Item == Patch);

	// In a fight it's in the Item slot; using it heals the run too, and it's gone.
	Mode->HandleUIConfirm();
	UIJPAbilityComponent* Abilities = Mode->GetArena()->GetPaddle(EIJPSide::Left)->GetAbilities();
	UTEST_TRUE("In hand for the fight", Abilities->HasItem());
	Mode->GetMatch()->ApplyDamage(EIJPSide::Left, 3.f);
	UTEST_EQUAL("Hurt", Run->GetHealth(), 2.f);
	UTEST_TRUE("Used", Abilities->TryActivate(EIJPAbilitySlot::Item));
	UTEST_EQUAL("The run healed", Run->GetHealth(), 4.f);
	UTEST_TRUE("Gone from the run", Run->GetLoadout().Item == nullptr);

	// A boss always drops one.
	Mode->GetMatch()->StopMatch();
	Run->CompleteNode(true);
	Run->EnterNode(Run->GetReachableNodes()[0]); // rest
	Run->EnterNode(Run->GetMap().BossIndex);
	Run->CompleteNode(true);
	UTEST_EQUAL("Won", Run->GetState(), EIJPRunState::Won);
	UTEST_TRUE("The boss dropped an item", Run->GetLoadout().Item == Patch);
	return true;
}

#endif
