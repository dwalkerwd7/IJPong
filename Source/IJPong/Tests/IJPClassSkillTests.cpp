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
