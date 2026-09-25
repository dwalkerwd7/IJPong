// It's Just Pong

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Abilities/IJPAbility_Catch.h"
#include "Abilities/IJPAbilityComponent.h"
#include "Core/IJPTypes.h"
#include "Gameplay/IJPArena.h"
#include "Gameplay/IJPBall.h"
#include "Gameplay/IJPBallType.h"
#include "Gameplay/IJPPaddle.h"
#include "Tests/IJPTestWorld.h"

namespace IJPCatchTests
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

	float AngleOf(const FVector2D& Velocity)
	{
		return static_cast<float>(FMath::RadiansToDegrees(FMath::Atan2(Velocity.Y, FMath::Abs(Velocity.X))));
	}

	/** The left paddle gets a catch (1 s hold, 70 deg limit), armed, and catches a straight serve. */
	UIJPAbility_Catch* CatchOne(FIJPTestWorld& Test, AIJPPaddle* Paddle, AIJPBall* Ball)
	{
		UIJPAbility_Catch* Catch = NewObject<UIJPAbility_Catch>(GetTransientPackage());
		Catch->HoldTime = 1.f;
		Catch->AimLimitDeg = 70.f;
		Paddle->GetAbilities()->Equip(EIJPAbilitySlot::ClassSkill, Catch);
		Paddle->GetAbilities()->TryActivate(EIJPAbilitySlot::ClassSkill);
		Ball->Serve(EIJPSide::Left, 0.f);
		RunUntil(Test, 2.f, [Ball] { return Ball->IsHeld(); });
		return Cast<UIJPAbility_Catch>(Paddle->GetAbilities()->GetAbility(EIJPAbilitySlot::ClassSkill));
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPCatchTest, "IJPong.ClassSkill.CatchHoldsCarriesAimsAndFires", IJPCatchTests::Flags)
bool FIJPCatchTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test;
	AIJPArena* Arena = Test.GetArena();
	AIJPPaddle* Paddle = Arena->GetPaddle(EIJPSide::Left);
	AIJPBall* Ball = Arena->GetBall();
	const float ServeSpeed = Ball->GetType().BaseSpeed;

	UIJPAbility_Catch* Catch = IJPCatchTests::CatchOne(Test, Paddle, Ball);
	UTEST_TRUE("Caught", Ball->IsHeld());
	UTEST_TRUE("Still in play", Ball->IsInPlay());
	UTEST_TRUE("Not moving", Ball->GetPlaneVelocity().IsNearlyZero());
	UTEST_TRUE("Holding", Catch->IsHolding());
	UTEST_EQUAL_TOLERANCE("The aim starts at the return's angle (straight)", Paddle->GetAimAngle(), 0.f, 0.5f);
	Test.Step();
	UTEST_TRUE("The aim line shows", Paddle->IsAimShown());

	// Carried: the ball rides the paddle.
	Test.RunFor(0.3f, [Paddle] { Paddle->AddMoveInput(1.f); });
	UTEST_TRUE("The paddle moved", Paddle->GetPlanePosition().Y > 20.f);
	UTEST_EQUAL_TOLERANCE("The ball came with it", static_cast<float>(Ball->GetPlanePosition().Y), static_cast<float>(Paddle->GetPlanePosition().Y), 1.f);

	// Aimed and fired on release, at the return's speed.
	Paddle->SetAimAngle(40.f);
	Paddle->GetAbilities()->Release(EIJPAbilitySlot::ClassSkill);
	UTEST_FALSE("Let go", Ball->IsHeld());
	UTEST_TRUE("Toward the other goal", Ball->GetPlaneVelocity().X > 0.f);
	UTEST_EQUAL_TOLERANCE("At the aim", IJPCatchTests::AngleOf(Ball->GetPlaneVelocity()), 40.f, 0.5f);
	UTEST_TRUE("At least the serve's speed", Ball->GetPlaneVelocity().Size() >= ServeSpeed - 1.f);
	Test.Step();
	UTEST_FALSE("The aim line's gone", Paddle->IsAimShown());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPCatchTimeoutTest, "IJPong.ClassSkill.CatchFiresItselfWithinTheAimLimit", IJPCatchTests::Flags)
bool FIJPCatchTimeoutTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test;
	AIJPArena* Arena = Test.GetArena();
	AIJPPaddle* Paddle = Arena->GetPaddle(EIJPSide::Left);
	AIJPBall* Ball = Arena->GetBall();

	// A stick pointing up and to the right is a 45-degree aim for the left paddle...
	Paddle->SetAimDirection(FVector2D(1.f, 1.f));
	UTEST_EQUAL_TOLERANCE("Stick aim", Paddle->GetAimAngle(), 45.f, 0.5f);
	// ...and up and to the left for the right one.
	AIJPPaddle* Right = Arena->GetPaddle(EIJPSide::Right);
	Right->SetAimDirection(FVector2D(-1.f, 1.f));
	UTEST_EQUAL_TOLERANCE("Mirrored for the right paddle", Right->GetAimAngle(), 45.f, 0.5f);

	IJPCatchTests::CatchOne(Test, Paddle, Ball);
	UTEST_TRUE("Caught", Ball->IsHeld());
	Paddle->SetAimAngle(-89.f);

	// Held too long: it goes by itself, no steeper than the limit.
	UTEST_TRUE("Fires itself", IJPCatchTests::RunUntil(Test, 1.2f, [Ball] { return !Ball->IsHeld(); }));
	UTEST_EQUAL_TOLERANCE("Clamped to the limit", IJPCatchTests::AngleOf(Ball->GetPlaneVelocity()), -70.f, 0.5f);

	// The steep shot survives a wall bounce (mirrored, not flattened back to a normal return).
	UTEST_TRUE("Off the wall", IJPCatchTests::RunUntil(Test, 2.f, [Ball] { return Ball->GetPlaneVelocity().Y > 0.f; }));
	UTEST_EQUAL_TOLERANCE("Still steep", IJPCatchTests::AngleOf(Ball->GetPlaneVelocity()), 70.f, 0.5f);
	return true;
}

#endif
