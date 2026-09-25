// It's Just Pong

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Abilities/IJPAbility_Grow.h"
#include "Abilities/IJPAbility_Smash.h"
#include "Abilities/IJPAbilityComponent.h"
#include "Core/IJPTypes.h"
#include "Gameplay/IJPArena.h"
#include "Gameplay/IJPBall.h"
#include "Gameplay/IJPBallType.h"
#include "Gameplay/IJPPaddle.h"
#include "Gameplay/IJPPaddleProfile.h"
#include "Tests/IJPTestWorld.h"

namespace IJPAbilityTests
{
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter;

	bool RunUntil(FIJPTestWorld& Test, float MaxSeconds, TFunctionRef<bool()> Condition)
	{
		const int32 Steps = FMath::CeilToInt(MaxSeconds / FIJPTestWorld::FixedStep);
		for (int32 i = 0; i < Steps && !Condition(); ++i)
		{
			Test.Step();
		}
		return Condition();
	}

	UIJPAbility_Grow* MakeGrow(float Duration, float Cooldown)
	{
		UIJPAbility_Grow* Grow = NewObject<UIJPAbility_Grow>(GetTransientPackage());
		Grow->Duration = Duration;
		Grow->Cooldown = Cooldown;
		return Grow;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPSmashTest, "IJPong.Ability.SmashSpeedsUpOnlyTheNextReturn", IJPAbilityTests::Flags)
bool FIJPSmashTest::RunTest(const FString& Parameters)
{
	// The left paddle's class comes with Smash (x2), equipped from its profile at spawn.
	UIJPAbility_Smash* Smash = NewObject<UIJPAbility_Smash>(GetTransientPackage());
	Smash->SpeedMultiplier = 2.f;
	UIJPPaddleProfile* Profile = NewObject<UIJPPaddleProfile>(GetTransientPackage());
	Profile->ClassSkill = Smash;
	FIJPTestWorld Test(FTransform::Identity, [Profile](AIJPArena& Arena) { Arena.SetPaddleProfile(EIJPSide::Left, Profile); });

	AIJPArena* Arena = Test.GetArena();
	AIJPBall* Ball = Arena->GetBall();
	AIJPPaddle* Left = Arena->GetPaddle(EIJPSide::Left);
	UIJPAbilityComponent* Abilities = Left->GetAbilities();
	const UIJPAbility* Equipped = Abilities->GetAbility(EIJPAbilitySlot::ClassSkill);
	UTEST_TRUE("Profile's class skill equipped", Equipped && Equipped->IsA<UIJPAbility_Smash>());
	UTEST_TRUE("As the slot's own copy", Equipped != Smash);

	// Both paddles stand still in the middle, so the ball goes straight back and forth.
	Arena->GetPaddle(EIJPSide::Right)->GetController()->UnPossess();
	Test.RunFor(1.1f); // past the match's first serve
	Ball->Serve(EIJPSide::Left, 0.f);
	const float Served = Ball->GetPlaneVelocity().Size();

	UTEST_TRUE("Activates", Abilities->TryActivate(EIJPAbilitySlot::ClassSkill));
	UTEST_TRUE("Armed", Abilities->GetAbility(EIJPAbilitySlot::ClassSkill)->IsActive());
	UTEST_FALSE("Can't use again while cooling down", Abilities->TryActivate(EIJPAbilitySlot::ClassSkill));

	UTEST_TRUE("Left returns it", IJPAbilityTests::RunUntil(Test, 3.f, [Ball] { return Ball->GetRallyHits() >= 1; }));
	const float PerHit = Ball->GetType().SpeedPerHit;
	UTEST_EQUAL_TOLERANCE("Smashed: double the normal return", Ball->GetPlaneVelocity().Size(), (Served + PerHit) * 2.0, 0.5);
	UTEST_FALSE("Used up", Abilities->GetAbility(EIJPAbilitySlot::ClassSkill)->IsActive());

	UTEST_TRUE("Right returns it", IJPAbilityTests::RunUntil(Test, 3.f, [Ball] { return Ball->GetRallyHits() >= 2; }));
	UTEST_EQUAL_TOLERANCE("Back on the rally's normal speed", Ball->GetPlaneVelocity().Size(), Served + 2.0 * PerHit, 0.5);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPGrowTest, "IJPong.Ability.GrowLengthensThePaddleThenReverts", IJPAbilityTests::Flags)
bool FIJPGrowTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test;
	AIJPArena* Arena = Test.GetArena();
	AIJPPaddle* Left = Arena->GetPaddle(EIJPSide::Left);
	UIJPAbilityComponent* Abilities = Left->GetAbilities();
	Abilities->Equip(EIJPAbilitySlot::RunAbility, IJPAbilityTests::MakeGrow(1.f, 2.f));

	// Up against the top wall, so growing has to push the paddle down to fit.
	Test.RunFor(0.6f, [Left] { Left->AddMoveInput(1.f); });
	const double NormalLength = Left->GetSize().Y;
	UTEST_TRUE("Activates", Abilities->TryActivate(EIJPAbilitySlot::RunAbility));
	UTEST_EQUAL_TOLERANCE("Double length", Left->GetSize().Y, NormalLength * 2.0, 0.01);
	UTEST_TRUE("Still inside the walls", Left->GetPlanePosition().Y + Left->GetSize().Y * 0.5 <= Arena->GetHalfExtents().Y + 0.01);

	Test.RunFor(1.1f);
	UTEST_EQUAL_TOLERANCE("Back to normal after the duration", Left->GetSize().Y, NormalLength, 0.01);
	UTEST_FALSE("Effect over", Abilities->GetAbility(EIJPAbilitySlot::RunAbility)->IsActive());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPCooldownTest, "IJPong.Ability.CooldownGatesEachSlot", IJPAbilityTests::Flags)
bool FIJPCooldownTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test;
	UIJPAbilityComponent* Abilities = Test.GetArena()->GetPaddle(EIJPSide::Left)->GetAbilities();
	Abilities->Equip(EIJPAbilitySlot::RunAbility, IJPAbilityTests::MakeGrow(0.5f, 2.f));

	Abilities->Equip(EIJPAbilitySlot::ClassSkill, nullptr);
	UTEST_FALSE("An empty slot does nothing", Abilities->TryActivate(EIJPAbilitySlot::ClassSkill));
	UTEST_TRUE("First use", Abilities->TryActivate(EIJPAbilitySlot::RunAbility));
	Test.RunFor(1.f);
	UTEST_FALSE("Effect over, but still cooling down", Abilities->TryActivate(EIJPAbilitySlot::RunAbility));
	UTEST_TRUE("Cooldown counting down", Abilities->GetCooldownRemaining(EIJPAbilitySlot::RunAbility) > 0.f);
	Test.RunFor(1.1f);
	UTEST_TRUE("Ready again", Abilities->IsReady(EIJPAbilitySlot::RunAbility));
	UTEST_TRUE("Second use", Abilities->TryActivate(EIJPAbilitySlot::RunAbility));
	return true;
}

#endif
