// It's Just Pong

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Abilities/IJPAbility_Freeze.h"
#include "Abilities/IJPAbility_Grow.h"
#include "Abilities/IJPAbility_SlowMo.h"
#include "Abilities/IJPAbilityComponent.h"
#include "Core/IJPRunGameMode.h"
#include "Core/IJPTypes.h"
#include "Engine/World.h"
#include "Gameplay/IJPArena.h"
#include "Gameplay/IJPBall.h"
#include "Gameplay/IJPBallType.h"
#include "Gameplay/IJPMatchComponent.h"
#include "Gameplay/IJPMatchRules.h"
#include "Gameplay/IJPPaddle.h"
#include "Run/IJPActConfig.h"
#include "Run/IJPReward.h"
#include "Run/IJPRunMapView.h"
#include "Run/IJPRunSubsystem.h"
#include "Tests/IJPTestWorld.h"

namespace IJPRewardTests
{
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter;

	/** Match, Rest, Boss in a straight line; every fight first to 1 with a quick serve. */
	UIJPActConfig* MakeStraightAct()
	{
		UIJPActConfig* Act = NewObject<UIJPActConfig>(GetTransientPackage());
		Act->Rows = 3;
		Act->Lanes = 1;
		Act->Paths = 1;
		Act->SkipCoins = 7;
		UIJPMatchRules* OnePoint = NewObject<UIJPMatchRules>(Act);
		OnePoint->StartingHealth = 1.f;
		OnePoint->ServeDelay = 0.2f;
		for (FIJPEncounter* Encounter : { &Act->Match, &Act->Elite, &Act->Boss })
		{
			Encounter->Rules = OnePoint;
			Encounter->Coins = 10;
		}
		return Act;
	}

	UIJPReward_Modifier* MakeModifier(EIJPRunStat Stat, float Amount)
	{
		UIJPReward_Modifier* Reward = NewObject<UIJPReward_Modifier>(GetTransientPackage());
		Reward->Stat = Stat;
		Reward->Amount = Amount;
		return Reward;
	}

	/** Start a run of Act and win its first match (on paper). */
	void WinFirstMatch(UIJPRunSubsystem* Run, const UIJPActConfig* Act, int32 Health = 5)
	{
		Run->StartRun(Act, 1, Health);
		Run->EnterNode(Run->GetReachableNodes()[0]);
		Run->CompleteNode(true);
	}

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
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPRewardOfferTest, "IJPong.Run.WinsOfferAPickOrCoins", IJPRewardTests::Flags)
bool FIJPRewardOfferTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test;
	UIJPRunSubsystem* Run = UIJPRunSubsystem::Get(Test.GetWorld());
	UIJPActConfig* Act = IJPRewardTests::MakeStraightAct();
	UIJPReward_Modifier* Tougher = IJPRewardTests::MakeModifier(EIJPRunStat::MaxHealth, 2.f);
	Act->MatchRewards = { IJPRewardTests::MakeModifier(EIJPRunStat::PaddleLength, 0.1f), IJPRewardTests::MakeModifier(EIJPRunStat::PaddleSpeed, 0.1f),
		Tougher, IJPRewardTests::MakeModifier(EIJPRunStat::ClassSkillCooldown, 0.15f) };

	IJPRewardTests::WinFirstMatch(Run, Act);
	UTEST_TRUE("A win offers a pick", Run->HasOffer());
	UTEST_EQUAL("Three to choose from", Run->GetOffer().Num(), 3);
	UTEST_TRUE("All different", Run->GetOffer()[0] != Run->GetOffer()[1] && Run->GetOffer()[1] != Run->GetOffer()[2] && Run->GetOffer()[0] != Run->GetOffer()[2]);
	UTEST_TRUE("The map waits for the pick", Run->GetReachableNodes().IsEmpty());

	Run->TakeReward(INDEX_NONE);
	UTEST_FALSE("Skipped", Run->HasOffer());
	UTEST_EQUAL("Skipping pays coins (on top of the win's)", Run->GetCoins(), 17);
	UTEST_FALSE("The map is open again", Run->GetReachableNodes().IsEmpty());

	// Taking one applies it for the run.
	Act->MatchRewards = { Tougher };
	IJPRewardTests::WinFirstMatch(Run, Act, 5);
	Run->LoseHealth(1);
	Run->TakeReward(0);
	UTEST_EQUAL("Tougher: max health up", Run->GetMaxHealth(), 7.f);
	UTEST_EQUAL("And healed the same", Run->GetHealth(), 6.f);

	// A loss offers nothing.
	Run->StartRun(Act, 1, 5);
	Run->EnterNode(Run->GetReachableNodes()[0]);
	Run->CompleteNode(false);
	UTEST_FALSE("No pick after a loss", Run->HasOffer());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPRewardAbilityTest, "IJPong.Run.AbilityRewardFillsTheRunSlot", IJPRewardTests::Flags)
bool FIJPRewardAbilityTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test;
	UIJPRunSubsystem* Run = UIJPRunSubsystem::Get(Test.GetWorld());
	UIJPActConfig* Act = IJPRewardTests::MakeStraightAct();
	UIJPReward_Ability* GetFreeze = NewObject<UIJPReward_Ability>(GetTransientPackage());
	GetFreeze->Ability = NewObject<UIJPAbility_Freeze>(GetTransientPackage());
	UIJPReward_Ability* GetGrow = NewObject<UIJPReward_Ability>(GetTransientPackage());
	GetGrow->Ability = NewObject<UIJPAbility_Grow>(GetTransientPackage());
	Act->MatchRewards = { GetFreeze };

	IJPRewardTests::WinFirstMatch(Run, Act);
	Run->TakeReward(0);
	UTEST_EQUAL("Freeze in the run slot", Run->GetLoadout().RunAbility.Get(), static_cast<const UIJPAbility*>(GetFreeze->Ability.Get()));
	UTEST_FALSE("Not offered again while held", GetFreeze->CanOffer(*Run));
	UTEST_TRUE("Others still are", GetGrow->CanOffer(*Run));

	GetGrow->Grant(*Run);
	UTEST_EQUAL("A new one replaces it", Run->GetLoadout().RunAbility.Get(), static_cast<const UIJPAbility*>(GetGrow->Ability.Get()));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPLoadoutTest, "IJPong.Run.LoadoutCarriesIntoEveryFight", IJPRewardTests::Flags)
bool FIJPLoadoutTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test(FTransform::Identity, nullptr, AIJPRunGameMode::StaticClass());
	AIJPRunGameMode* Mode = Cast<AIJPRunGameMode>(Test.GetWorld()->GetAuthGameMode());
	UIJPRunSubsystem* Run = UIJPRunSubsystem::Get(Test.GetWorld());
	AIJPArena* Arena = Mode->GetArena();
	AIJPPaddle* Left = Arena->GetPaddle(EIJPSide::Left);
	AIJPPaddle* Right = Arena->GetPaddle(EIJPSide::Right);
	const double BaseLength = Left->GetSize().Y;

	UIJPActConfig* Act = IJPRewardTests::MakeStraightAct();
	Act->MatchRewards = { IJPRewardTests::MakeModifier(EIJPRunStat::PaddleLength, 0.5f) };
	Mode->PostMatchDelay = 0.2f;
	Mode->StartNewRun(Act, 1, 5);

	// Win the first match: the opponent steps aside and the ball goes into their goal.
	Mode->HandleUIConfirm();
	Right->GetController()->UnPossess();
	const auto HoldRightUp = [Right] { Right->AddMoveInput(1.f); };
	Test.RunFor(0.5f, HoldRightUp);
	Arena->GetBall()->Serve(EIJPSide::Right, 0.f);
	UTEST_TRUE("Reward screen after the win", IJPRewardTests::RunUntil(Test, 3.f, [Mode] { return Mode->GetPhase() == EIJPRunPhase::Reward; }, HoldRightUp));
	UTEST_TRUE("Cards on screen", Mode->GetMapView()->IsShowingCards());
	UTEST_TRUE("Take the first card", Mode->HandleUIConfirm());
	UTEST_EQUAL("Back on the map", Mode->GetPhase(), EIJPRunPhase::Map);
	UTEST_EQUAL("Took it", Run->GetLoadout().PaddleLengthBonus, 0.5f);

	// More gathered along the way (as other picks would): a ball and a run ability.
	UIJPBallType* Heavy = NewObject<UIJPBallType>(GetTransientPackage());
	Run->EditLoadout().ExtraServedBalls.Add(Heavy);
	Run->EditLoadout().RunAbility = NewObject<UIJPAbility_Freeze>(GetTransientPackage());

	Mode->HandleUIConfirm(); // rest
	Mode->HandleUIConfirm(); // boss
	UTEST_EQUAL("Fighting the boss", Mode->GetPhase(), EIJPRunPhase::Playing);
	UTEST_EQUAL_TOLERANCE("Longer paddle", Left->GetSize().Y, BaseLength * 1.5, 0.01);
	const UIJPAbility* RunSlot = Left->GetAbilities()->GetAbility(EIJPAbilitySlot::RunAbility);
	UTEST_TRUE("Run ability equipped", RunSlot && RunSlot->IsA<UIJPAbility_Freeze>());
	UTEST_TRUE("Serves carry the extra ball", IJPRewardTests::RunUntil(Test, 1.f, [Arena] { return Arena->GetNumBallsInPlay() == 2; }));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPFreezeTest, "IJPong.Ability.FreezeStopsEveryBall", IJPRewardTests::Flags)
bool FIJPFreezeTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test;
	AIJPArena* Arena = Test.GetArena();
	Test.RunFor(1.1f);
	AIJPBall* Main = Arena->GetBall();
	AIJPBall* Other = Arena->AddBall(nullptr);
	Main->Serve(EIJPSide::Left, 20.f);
	Other->Serve(EIJPSide::Right, -20.f);
	Test.RunFor(0.1f);

	UIJPAbilityComponent* Abilities = Arena->GetPaddle(EIJPSide::Left)->GetAbilities();
	Abilities->Equip(EIJPAbilitySlot::RunAbility, NewObject<UIJPAbility_Freeze>(GetTransientPackage()));
	UTEST_TRUE("Freeze", Abilities->TryActivate(EIJPAbilitySlot::RunAbility));
	const FVector2D MainAt = Main->GetPlanePosition();
	const FVector2D OtherAt = Other->GetPlanePosition();
	Test.RunFor(0.3f);
	UTEST_TRUE("Both frozen in place", Main->GetPlanePosition().Equals(MainAt) && Other->GetPlanePosition().Equals(OtherAt));

	Test.RunFor(0.4f);
	UTEST_FALSE("Moving again", Main->GetPlanePosition().Equals(MainAt));
	UTEST_TRUE("Same heading as before", Main->GetPlanePosition().X < MainAt.X);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPSlowMoTest, "IJPong.Ability.SlowMoSlowsBallsNotPaddles", IJPRewardTests::Flags)
bool FIJPSlowMoTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test;
	AIJPArena* Arena = Test.GetArena();
	AIJPPaddle* Left = Arena->GetPaddle(EIJPSide::Left);
	Arena->GetPaddle(EIJPSide::Right)->GetController()->UnPossess();
	Test.RunFor(1.1f);
	AIJPBall* Ball = Arena->GetBall();
	Ball->Serve(EIJPSide::Right, 0.f);

	auto Travel = [&Test, Ball](float Seconds)
	{
		const double From = Ball->GetPlanePosition().X;
		Test.RunFor(Seconds);
		return Ball->GetPlanePosition().X - From;
	};
	const double Normal = Travel(0.2f);

	UIJPAbility_SlowMo* SlowMo = NewObject<UIJPAbility_SlowMo>(GetTransientPackage());
	SlowMo->Duration = 1.f;
	UIJPAbilityComponent* Abilities = Left->GetAbilities();
	Abilities->Equip(EIJPAbilitySlot::RunAbility, SlowMo);
	UTEST_TRUE("Slow-mo", Abilities->TryActivate(EIJPAbilitySlot::RunAbility));
	const double Slowed = Travel(0.2f);
	UTEST_EQUAL_TOLERANCE("Ball at half speed", Slowed, Normal * 0.5, Normal * 0.1);

	// The paddle still runs at full speed meanwhile.
	const double PaddleFrom = Left->GetPlanePosition().Y;
	Test.RunFor(0.2f, [Left] { Left->AddMoveInput(1.f); });
	UTEST_TRUE("Paddle unaffected", Left->GetPlanePosition().Y - PaddleFrom > Left->GetMaxSpeed() * 0.2 * 0.8);

	Test.RunFor(1.f);
	UTEST_EQUAL("Back to normal after", Ball->GetTimeScale(), 1.f);
	return true;
}

#endif
