// It's Just Pong

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Abilities/IJPAbility_Grow.h"
#include "Abilities/IJPAbility_Smash.h"
#include "Abilities/IJPAbilityComponent.h"
#include "Audio/IJPToneSet.h"
#include "Audio/IJPToneSynthComponent.h"
#include "Core/IJPTypes.h"
#include "Gameplay/IJPArena.h"
#include "Gameplay/IJPBall.h"
#include "Gameplay/IJPBallType.h"
#include "Gameplay/IJPPaddle.h"
#include "Gameplay/IJPPaddleClass.h"
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
	// The left paddle's class comes with Smash (x2), equipped from the class at spawn.
	UIJPAbility_Smash* Smash = NewObject<UIJPAbility_Smash>(GetTransientPackage());
	Smash->SpeedMultiplier = 2.f;
	UIJPPaddleClass* PaddleClass = NewObject<UIJPPaddleClass>(GetTransientPackage());
	PaddleClass->ClassSkill = Smash;
	FIJPTestWorld Test(FTransform::Identity, [PaddleClass](AIJPArena& Arena) { Arena.SetPaddleClass(EIJPSide::Left, PaddleClass); });

	AIJPArena* Arena = Test.GetArena();
	AIJPBall* Ball = Arena->GetBall();
	AIJPPaddle* Left = Arena->GetPaddle(EIJPSide::Left);
	UIJPAbilityComponent* Abilities = Left->GetAbilities();
	const UIJPAbility* Equipped = Abilities->GetAbility(EIJPAbilitySlot::ClassSkill);
	UTEST_TRUE("Class skill equipped", Equipped && Equipped->IsA<UIJPAbility_Smash>());
	UTEST_TRUE("As the slot's own copy", Equipped != Smash);

	// Both paddles stand still in the middle, so the ball goes straight back and forth.
	Arena->GetPaddle(EIJPSide::Right)->GetController()->UnPossess();
	Test.RunFor(1.1f); // past the match's first serve
	Ball->Serve(EIJPSide::Left, 0.f);
	const float Served = Ball->GetPlaneVelocity().Size();

	UTEST_TRUE("Activates", Abilities->TryActivate(EIJPAbilitySlot::ClassSkill));
	UTEST_TRUE("Armed", Abilities->GetAbility(EIJPAbilitySlot::ClassSkill)->IsActive());

	UTEST_TRUE("Left returns it", IJPAbilityTests::RunUntil(Test, 3.f, [Ball] { return Ball->GetRallyHits() >= 1; }));
	const float PerHit = Ball->GetType().SpeedPerHit;
	UTEST_EQUAL_TOLERANCE("Smashed: double the normal return", Ball->GetPlaneVelocity().Size(), (Served + PerHit) * 2.0, 0.5);
	UTEST_FALSE("Used up", Abilities->GetAbility(EIJPAbilitySlot::ClassSkill)->IsActive());
	UTEST_FALSE("Can't use again while cooling down", Abilities->TryActivate(EIJPAbilitySlot::ClassSkill));

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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPArmedCueTest, "IJPong.Ability.ArmedSkillPulsesAndChirps", IJPAbilityTests::Flags)
bool FIJPArmedCueTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test;
	AIJPArena* Arena = Test.GetArena();
	AIJPBall* Ball = Arena->GetBall();
	AIJPPaddle* Left = Arena->GetPaddle(EIJPSide::Left);
	UIJPAbilityComponent* Abilities = Left->GetAbilities();
	UIJPToneSynthComponent* Tones = Arena->GetTones();
	const UIJPToneSet& ToneSet = Arena->GetToneSet();
	Abilities->Equip(EIJPAbilitySlot::ClassSkill, NewObject<UIJPAbility_Smash>(GetTransientPackage()));
	Abilities->Equip(EIJPAbilitySlot::RunAbility, IJPAbilityTests::MakeGrow(1.f, 5.f));
	Arena->GetPaddle(EIJPSide::Right)->GetController()->UnPossess();
	Test.RunFor(1.1f);

	// A timed effect isn't "armed": no cue.
	UTEST_TRUE("Grow on", Abilities->TryActivate(EIJPAbilitySlot::RunAbility));
	Test.Step();
	UTEST_FALSE("No cue for a timed effect", Left->IsArmedCueShown());

	// Arming: a two-note rising chirp and a pulsing halo.
	UTEST_TRUE("Smash armed", Abilities->TryActivate(EIJPAbilitySlot::ClassSkill));
	UTEST_TRUE("First note", Tones->GetLastTone() == ToneSet.Arm);
	Test.Step();
	UTEST_TRUE("Cue up", Left->IsArmedCueShown());
	const float Bright = Left->GetArmedCueStrength();
	Test.RunFor(1.f / 6.f); // half a pulse at 3 Hz
	const float Dim = Left->GetArmedCueStrength();
	UTEST_TRUE("It pulses", Bright - Dim > 0.2f);
	UTEST_TRUE("Second note, higher", Tones->GetLastTone().Frequency > ToneSet.Arm.Frequency);

	// The smashed return uses it up, and the cue goes.
	Ball->Serve(EIJPSide::Left, 0.f);
	UTEST_TRUE("Returned", IJPAbilityTests::RunUntil(Test, 2.f, [Ball] { return Ball->GetRallyHits() >= 1; }));
	Test.Step();
	UTEST_FALSE("Cue gone after the hit", Left->IsArmedCueShown());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPReadyCueTest, "IJPong.Ability.PlayerSlotReadyFlashesAndBlips", IJPAbilityTests::Flags)
bool FIJPReadyCueTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test;
	AIJPArena* Arena = Test.GetArena();
	AIJPPaddle* Left = Arena->GetPaddle(EIJPSide::Left);
	UIJPAbilityComponent* Abilities = Left->GetAbilities();
	UIJPToneSynthComponent* Tones = Arena->GetTones();
	const UIJPToneSet& ToneSet = Arena->GetToneSet();
	Abilities->Equip(EIJPAbilitySlot::ClassSkill, nullptr);
	Abilities->Equip(EIJPAbilitySlot::RunAbility, IJPAbilityTests::MakeGrow(0.2f, 1.f));
	Test.Step();
	UTEST_FALSE("No flash before anything cools", Left->IsArmedCueShown());

	// The run ability comes back: one flash and the run slot's blip.
	UTEST_TRUE("Used", Abilities->TryActivate(EIJPAbilitySlot::RunAbility));
	const int32 Before = Abilities->GetReadySignals();
	UTEST_TRUE("Back", IJPAbilityTests::RunUntil(Test, 1.5f, [Abilities, Before] { return Abilities->GetReadySignals() > Before; }));
	UTEST_EQUAL("Once", Abilities->GetReadySignals(), Before + 1);
	const float RunPitch = ToneSet.Ready.Frequency * ToneSet.ReadySlotPitch[static_cast<int32>(EIJPAbilitySlot::RunAbility)];
	UTEST_TRUE("The run slot's blip", FMath::IsNearlyEqual(Tones->GetLastTone().Frequency, RunPitch, 0.1f));
	Test.Step();
	UTEST_TRUE("Flashing", Left->IsArmedCueShown());
	Test.RunFor(0.5f); // well past ReadyFlashTime
	UTEST_FALSE("Just a flash", Left->IsArmedCueShown());

	// The AI's slots come back silently.
	AIJPPaddle* Right = Arena->GetPaddle(EIJPSide::Right);
	UIJPAbilityComponent* RightAbilities = Right->GetAbilities();
	RightAbilities->Equip(EIJPAbilitySlot::ClassSkill, nullptr);
	RightAbilities->Equip(EIJPAbilitySlot::RunAbility, IJPAbilityTests::MakeGrow(0.2f, 0.5f));
	UTEST_TRUE("AI used it", RightAbilities->TryActivate(EIJPAbilitySlot::RunAbility));
	Test.RunFor(1.f);
	UTEST_EQUAL("No signal for the AI", RightAbilities->GetReadySignals(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPToggleTest, "IJPong.Ability.ArmedSkillTogglesOffWithRefund", IJPAbilityTests::Flags)
bool FIJPToggleTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test;
	AIJPArena* Arena = Test.GetArena();
	AIJPBall* Ball = Arena->GetBall();
	AIJPPaddle* Left = Arena->GetPaddle(EIJPSide::Left);
	UIJPAbilityComponent* Abilities = Left->GetAbilities();
	UIJPToneSynthComponent* Tones = Arena->GetTones();
	const UIJPToneSet& ToneSet = Arena->GetToneSet();
	UIJPAbility_Smash* Smash = NewObject<UIJPAbility_Smash>(GetTransientPackage());
	Smash->Cooldown = 5.f;
	Abilities->Equip(EIJPAbilitySlot::ClassSkill, Smash);
	Arena->GetPaddle(EIJPSide::Right)->GetController()->UnPossess();
	Test.RunFor(1.1f);

	// Arm, then press again: disarmed, cooldown back, a falling chirp.
	UTEST_TRUE("Armed", Abilities->TryActivate(EIJPAbilitySlot::ClassSkill));
	UTEST_TRUE("Cooling", Abilities->GetCooldownRemaining(EIJPAbilitySlot::ClassSkill) > 0.f);
	UTEST_FALSE("A second press isn't a use", Abilities->TryActivate(EIJPAbilitySlot::ClassSkill));
	UTEST_FALSE("Disarmed", Abilities->IsArmed());
	UTEST_EQUAL("Cooldown refunded", Abilities->GetCooldownRemaining(EIJPAbilitySlot::ClassSkill), 0.f);
	UTEST_TRUE("High note first", Tones->GetLastTone().Frequency > ToneSet.Arm.Frequency);
	Test.RunFor(ToneSet.Arm.Duration + 0.05f);
	UTEST_TRUE("Then the low one", Tones->GetLastTone() == ToneSet.Arm);

	// The next return is a plain one.
	Ball->Serve(EIJPSide::Left, 0.f);
	const float ServeSpeed = Ball->GetPlaneVelocity().Size();
	UTEST_TRUE("Returned", IJPAbilityTests::RunUntil(Test, 2.f, [Ball] { return Ball->GetRallyHits() >= 1; }));
	UTEST_TRUE("Not smashed", Ball->GetPlaneVelocity().Size() < ServeSpeed * 1.3f);

	// And it can be armed again straight away.
	UTEST_TRUE("Ready again", Abilities->TryActivate(EIJPAbilitySlot::ClassSkill));
	UTEST_TRUE("Armed again", Abilities->IsArmed());
	return true;
}

#endif
