// It's Just Pong

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Abilities/IJPAbility_Barrier.h"
#include "Abilities/IJPAbility_Curve.h"
#include "Abilities/IJPAbility_Dash.h"
#include "Abilities/IJPAbility_Split.h"
#include "Abilities/IJPAbilityComponent.h"
#include "Core/IJPTestGameMode.h"
#include "Core/IJPTypes.h"
#include "Engine/World.h"
#include "Gameplay/IJPArena.h"
#include "Gameplay/IJPBall.h"
#include "Gameplay/IJPMatchComponent.h"
#include "Gameplay/IJPPaddle.h"
#include "Tests/IJPTestWorld.h"

namespace IJPClassSkillTests
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

	/** Degrees between the ball's path and horizontal. */
	double SlopeDeg(const AIJPBall* Ball)
	{
		const FVector2D V = Ball->GetPlaneVelocity();
		return FMath::RadiansToDegrees(FMath::Atan2(FMath::Abs(V.Y), FMath::Abs(V.X)));
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPDashTest, "IJPong.Ability.DashBurstsPastTopSpeed", IJPClassSkillTests::Flags)
bool FIJPDashTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test;
	AIJPPaddle* Left = Test.GetArena()->GetPaddle(EIJPSide::Left);
	UIJPAbility_Dash* Dash = NewObject<UIJPAbility_Dash>(GetTransientPackage());
	Dash->Distance = 100.f;
	Dash->Duration = 0.1f;
	Left->GetAbilities()->Equip(EIJPAbilitySlot::ClassSkill, Dash);

	// Steer up briefly, then let go: the dash goes the way the paddle was last steered.
	Test.RunFor(0.1f, [Left] { Left->AddMoveInput(1.f); });
	Test.RunFor(0.3f);
	const double StartY = Left->GetPlanePosition().Y;

	UTEST_TRUE("Dashes", Left->GetAbilities()->TryActivate(EIJPAbilitySlot::ClassSkill));
	Test.RunFor(0.12f);
	const double Moved = Left->GetPlanePosition().Y - StartY;
	UTEST_TRUE("Covered the dash distance, upward", Moved >= 95.0);
	UTEST_TRUE("Faster than it can run", Moved > Left->GetMaxSpeed() * 0.12);
	UTEST_FALSE("Burst over", Left->IsDashing());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPDashTreeTest, "IJPong.Tree.StrikerUpgradesReachDoubleDashSlipstreamStrike", IJPClassSkillTests::Flags)
bool FIJPDashTreeTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test;
	AIJPArena* Arena = Test.GetArena();
	AIJPPaddle* Left = Arena->GetPaddle(EIJPSide::Left);
	UIJPAbilityComponent* Abilities = Left->GetAbilities();
	UIJPAbility_Dash* Dash = NewObject<UIJPAbility_Dash>(GetTransientPackage());
	Dash->Distance = 100.f;
	Dash->Duration = 0.1f;
	Abilities->Equip(EIJPAbilitySlot::ClassSkill, Dash);
	Abilities->GetAbility(EIJPAbilitySlot::ClassSkill)->SetUpgrades({ { FName(TEXT("Distance")), 0.5f }, { FName(TEXT("ExtraDashes")), 1.f }, { FName(TEXT("Slipstream")), 1.f }, { FName(TEXT("DashStrike")), 1.f } });
	const float BaseSpeed = Left->GetMaxSpeed();
	Test.RunFor(1.1f);

	// Reach: half as far again. Slipstream: faster for a while after.
	Test.RunFor(0.1f, [Left] { Left->AddMoveInput(-1.f); });
	Test.RunFor(0.3f);
	const double StartY = Left->GetPlanePosition().Y;
	UTEST_TRUE("Dashes", Abilities->TryActivate(EIJPAbilitySlot::ClassSkill));
	Test.RunFor(0.12f);
	UTEST_TRUE("Further", StartY - Left->GetPlanePosition().Y >= 145.0);
	UTEST_TRUE("Slipstream", Left->GetMaxSpeed() > BaseSpeed * 1.25f);

	// Double Dash: the first was free, the second starts the cooldown.
	UTEST_TRUE("Still ready", Abilities->IsReady(EIJPAbilitySlot::ClassSkill));
	UTEST_TRUE("Second dash", Abilities->TryActivate(EIJPAbilitySlot::ClassSkill));
	UTEST_FALSE("Now cooling down", Abilities->IsReady(EIJPAbilitySlot::ClassSkill));
	Test.RunFor(1.1f);
	UTEST_EQUAL_TOLERANCE("Slipstream wears off", Left->GetMaxSpeed(), BaseSpeed, 0.01f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPDashStrikeTest, "IJPong.Tree.DashStrikeSmashesAReturnRightAfterADash", IJPClassSkillTests::Flags)
bool FIJPDashStrikeTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test;
	AIJPArena* Arena = Test.GetArena();
	AIJPBall* Ball = Arena->GetBall();
	AIJPPaddle* Left = Arena->GetPaddle(EIJPSide::Left);
	UIJPAbilityComponent* Abilities = Left->GetAbilities();
	UIJPAbility_Dash* Dash = NewObject<UIJPAbility_Dash>(GetTransientPackage());
	Dash->Distance = 1.f; // barely moves: the ball still meets it
	Abilities->Equip(EIJPAbilitySlot::ClassSkill, Dash);
	Abilities->GetAbility(EIJPAbilitySlot::ClassSkill)->SetUpgrades({ { FName(TEXT("DashStrike")), 1.f } });
	Test.RunFor(1.1f);

	// A ball a moment away from the paddle: dash, and the return is smashed.
	const FVector2D Face = Left->GetPlanePosition() + FVector2D(Left->GetSize().X * 0.5f, 0.f);
	Ball->Launch(Face + FVector2D(60.f, 0.f), FVector2D(-400.f, 0.f));
	Abilities->TryActivate(EIJPAbilitySlot::ClassSkill);
	UTEST_TRUE("Returned", IJPClassSkillTests::RunUntil(Test, 1.f, [Ball] { return Ball->GetRallyHits() >= 1; }));
	UTEST_TRUE("Smashed", Ball->IsBoosted());

	// Later, a plain return.
	Test.RunFor(1.f);
	Ball->Launch(Face + FVector2D(60.f, 0.f), FVector2D(-400.f, 0.f));
	UTEST_TRUE("Returned again", IJPClassSkillTests::RunUntil(Test, 1.f, [Ball] { return Ball->GetRallyHits() >= 1; }));
	UTEST_FALSE("Not smashed without a dash", Ball->IsBoosted());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPBarrierTest, "IJPong.Ability.BarrierBlocksTheGoalForItsDuration", IJPClassSkillTests::Flags)
bool FIJPBarrierTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test;
	AIJPTestGameMode* Mode = Cast<AIJPTestGameMode>(Test.GetWorld()->GetAuthGameMode());
	AIJPArena* Arena = Mode->GetArena();
	AIJPBall* Ball = Arena->GetBall();
	AIJPPaddle* Left = Arena->GetPaddle(EIJPSide::Left);
	const auto HoldUp = [Left] { Left->AddMoveInput(1.f); };
	UIJPAbility_Barrier* Barrier = NewObject<UIJPAbility_Barrier>(GetTransientPackage());
	Barrier->Duration = 2.f;
	Left->GetAbilities()->Equip(EIJPAbilitySlot::ClassSkill, Barrier);
	Test.RunFor(1.1f, HoldUp);

	// Paddle out of the way at the top, ball straight at the open goal, barrier up.
	const int32 RightBefore = Mode->GetMatch()->GetGoals(EIJPSide::Right);
	Ball->Serve(EIJPSide::Left, 0.f);
	UTEST_TRUE("Raises it", Left->GetAbilities()->TryActivate(EIJPAbilitySlot::ClassSkill));
	UTEST_TRUE("Barrier up", Arena->IsBarrierUp(EIJPSide::Left));

	UTEST_TRUE("Ball bounces back off it", IJPClassSkillTests::RunUntil(Test, 1.5f, [Ball] { return Ball->GetPlaneVelocity().X > 0.f; }, HoldUp));
	UTEST_TRUE("Still in play", Ball->IsInPlay());
	UTEST_EQUAL("No goal", Mode->GetMatch()->GetGoals(EIJPSide::Right), RightBefore);
	UTEST_EQUAL("Not a paddle return", Ball->GetRallyHits(), 0);

	Test.RunFor(1.2f, HoldUp); // the bounce came about 1 s in; the barrier lasts 2 s
	UTEST_FALSE("Gone after its duration", Arena->IsBarrierUp(EIJPSide::Left));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPBarrierTreeTest, "IJPong.Tree.BulwarkUpgradesLongerReboundMending", IJPClassSkillTests::Flags)
bool FIJPBarrierTreeTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test;
	AIJPTestGameMode* Mode = Cast<AIJPTestGameMode>(Test.GetWorld()->GetAuthGameMode());
	AIJPArena* Arena = Mode->GetArena();
	UIJPMatchComponent* Match = Mode->GetMatch();
	AIJPBall* Ball = Arena->GetBall();
	AIJPPaddle* Left = Arena->GetPaddle(EIJPSide::Left);
	const auto HoldUp = [Left] { Left->AddMoveInput(1.f); };
	UIJPAbility_Barrier* Barrier = NewObject<UIJPAbility_Barrier>(GetTransientPackage());
	Barrier->Duration = 2.f;
	Left->GetAbilities()->Equip(EIJPAbilitySlot::ClassSkill, Barrier);
	Left->GetAbilities()->GetAbility(EIJPAbilitySlot::ClassSkill)->SetUpgrades({ { FName(TEXT("Duration")), 1.f }, { FName(TEXT("Rebound")), 1.f }, { FName(TEXT("Mending")), 0.25f } });
	Test.RunFor(1.1f, HoldUp);
	Match->ApplyDamage(EIJPSide::Left, 1.f);
	const float HurtHealth = Match->GetHealth(EIJPSide::Left);

	Ball->Serve(EIJPSide::Left, 0.f);
	UTEST_TRUE("Raises it", Left->GetAbilities()->TryActivate(EIJPAbilitySlot::ClassSkill));
	UTEST_TRUE("Bounces back off it", IJPClassSkillTests::RunUntil(Test, 1.5f, [Ball] { return Ball->GetPlaneVelocity().X > 0.f; }, HoldUp));
	UTEST_TRUE("Rebound: boosted", Ball->IsBoosted());
	UTEST_EQUAL_TOLERANCE("Mending: healed", Match->GetHealth(EIJPSide::Left), HurtHealth + 0.25f, 0.001f);

	Test.RunFor(1.2f, HoldUp); // 2 s base would be over by now; 3 s with the upgrade isn't
	UTEST_TRUE("Lasts longer", Arena->IsBarrierUp(EIJPSide::Left));
	Test.RunFor(1.f, HoldUp);
	UTEST_FALSE("Then down", Arena->IsBarrierUp(EIJPSide::Left));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPLastStandTest, "IJPong.Tree.LastStandSavesOneGoalPerMatch", IJPClassSkillTests::Flags)
bool FIJPLastStandTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test;
	AIJPTestGameMode* Mode = Cast<AIJPTestGameMode>(Test.GetWorld()->GetAuthGameMode());
	AIJPArena* Arena = Mode->GetArena();
	UIJPMatchComponent* Match = Mode->GetMatch();
	AIJPBall* Ball = Arena->GetBall();
	AIJPPaddle* Left = Arena->GetPaddle(EIJPSide::Left);
	const auto HoldUp = [Left] { Left->AddMoveInput(1.f); };
	Left->GetAbilities()->Equip(EIJPAbilitySlot::ClassSkill, NewObject<UIJPAbility_Barrier>(GetTransientPackage()));
	Left->GetAbilities()->GetAbility(EIJPAbilitySlot::ClassSkill)->SetUpgrades({ { FName(TEXT("LastStand")), 1.f } });
	Test.RunFor(1.1f, HoldUp);

	// Paddle out of the way, never pressed: the barrier rises by itself, once.
	const int32 Before = Match->GetGoals(EIJPSide::Right);
	Ball->Serve(EIJPSide::Left, 0.f);
	UTEST_TRUE("Saved", IJPClassSkillTests::RunUntil(Test, 1.5f, [Ball] { return Ball->GetPlaneVelocity().X > 0.f || !Ball->IsInPlay(); }, HoldUp));
	UTEST_TRUE("Still in play", Ball->IsInPlay());
	UTEST_EQUAL("No goal", Match->GetGoals(EIJPSide::Right), Before);

	Test.RunFor(1.2f, HoldUp);
	Ball->Serve(EIJPSide::Left, 0.f);
	UTEST_TRUE("Not twice in a match", IJPClassSkillTests::RunUntil(Test, 1.5f, [Ball] { return !Ball->IsInPlay(); }, HoldUp));
	UTEST_EQUAL("A goal this time", Match->GetGoals(EIJPSide::Right), Before + 1);

	// A new match: ready again.
	Mode->RestartMatch();
	Test.RunFor(1.1f, HoldUp);
	Ball->Serve(EIJPSide::Left, 0.f);
	UTEST_TRUE("Saved again", IJPClassSkillTests::RunUntil(Test, 1.5f, [Ball] { return Ball->GetPlaneVelocity().X > 0.f || !Ball->IsInPlay(); }, HoldUp));
	UTEST_TRUE("Next match, saved", Ball->IsInPlay());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPCurveTest, "IJPong.Ability.CurveShotBendsTheReturn", IJPClassSkillTests::Flags)
bool FIJPCurveTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test;
	AIJPArena* Arena = Test.GetArena();
	AIJPBall* Ball = Arena->GetBall();
	AIJPPaddle* Left = Arena->GetPaddle(EIJPSide::Left);
	Left->GetAbilities()->Equip(EIJPAbilitySlot::ClassSkill, NewObject<UIJPAbility_Curve>(GetTransientPackage()));
	Test.RunFor(1.1f);

	// Standing still in the middle: a straight return that bends as it goes.
	UTEST_TRUE("Arms", Left->GetAbilities()->TryActivate(EIJPAbilitySlot::ClassSkill));
	Ball->Serve(EIJPSide::Left, 0.f);
	UTEST_TRUE("Returned", IJPClassSkillTests::RunUntil(Test, 2.f, [Ball] { return Ball->GetRallyHits() >= 1; }));
	UTEST_TRUE("Curving", Ball->IsCurving());
	UTEST_TRUE("Leaves nearly flat", IJPClassSkillTests::SlopeDeg(Ball) < 5.0);

	Test.RunFor(0.4f);
	UTEST_TRUE("Bent upward by now", IJPClassSkillTests::SlopeDeg(Ball) > 20.0 && Ball->GetPlaneVelocity().Y > 0.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPSplitTest, "IJPong.Ability.SplitFansTheReturnIntoTwoBalls", IJPClassSkillTests::Flags)
bool FIJPSplitTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test;
	AIJPArena* Arena = Test.GetArena();
	AIJPBall* Ball = Arena->GetBall();
	AIJPPaddle* Left = Arena->GetPaddle(EIJPSide::Left);
	Left->GetAbilities()->Equip(EIJPAbilitySlot::ClassSkill, NewObject<UIJPAbility_Split>(GetTransientPackage()));
	Test.RunFor(1.1f);

	UTEST_TRUE("Arms", Left->GetAbilities()->TryActivate(EIJPAbilitySlot::ClassSkill));
	Ball->Serve(EIJPSide::Left, 0.f);
	UTEST_TRUE("Returned", IJPClassSkillTests::RunUntil(Test, 2.f, [Ball] { return Ball->GetRallyHits() >= 1; }));
	Test.Step();
	UTEST_EQUAL("Two balls in play", Arena->GetNumBallsInPlay(), 2);

	const AIJPBall* Twin = nullptr;
	for (const AIJPBall* Each : Arena->GetBalls())
	{
		if (Each != Ball && Each->IsInPlay())
		{
			Twin = Each;
		}
	}
	UTEST_NOT_NULL("The twin", Twin);
	UTEST_TRUE("Both head away from the paddle", Ball->GetPlaneVelocity().X > 0.f && Twin->GetPlaneVelocity().X > 0.f);
	UTEST_TRUE("Fanned apart", Ball->GetPlaneVelocity().Y * Twin->GetPlaneVelocity().Y < 0.f);
	UTEST_EQUAL("Same type", &Twin->GetType(), &Ball->GetType());
	return true;
}

#endif
