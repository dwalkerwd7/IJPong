// It's Just Pong

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Abilities/IJPAbility_Breaker.h"
#include "Abilities/IJPAbility_Snare.h"
#include "Abilities/IJPAbilityComponent.h"
#include "Audio/IJPToneSet.h"
#include "Audio/IJPToneSynthComponent.h"
#include "Core/IJPTestGameMode.h"
#include "Core/IJPTypes.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "Gameplay/IJPArena.h"
#include "Gameplay/IJPBall.h"
#include "Gameplay/IJPMatchComponent.h"
#include "Gameplay/IJPPaddle.h"
#include "Gameplay/IJPRival.h"
#include "Tests/IJPTestWorld.h"

namespace IJPRivalSkillTests
{
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter;

	AIJPTestGameMode* GetMode(const FIJPTestWorld& Test)
	{
		return Cast<AIJPTestGameMode>(Test.GetWorld()->GetAuthGameMode());
	}

	UIJPRival* MakeSnareRival()
	{
		UIJPRival* Rival = NewObject<UIJPRival>(GetTransientPackage());
		UIJPAbility_Snare* Snare = NewObject<UIJPAbility_Snare>(Rival);
		Snare->Telegraph = 0.5f;
		Snare->Duration = 1.f;
		Snare->SpeedMultiplier = 0.5f;
		Rival->RivalSkill = Snare;
		return Rival;
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
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPSnareTest, "IJPong.RivalSkill.SnareTelegraphsThenSlows", IJPRivalSkillTests::Flags)
bool FIJPSnareTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test;
	AIJPTestGameMode* Mode = IJPRivalSkillTests::GetMode(Test);
	AIJPArena* Arena = Mode->GetArena();
	AIJPPaddle* Player = Arena->GetPaddle(EIJPSide::Left);
	AIJPPaddle* RivalPaddle = Arena->GetPaddle(EIJPSide::Right);
	const UIJPAbility* ClassSkill = RivalPaddle->GetAbilities()->GetAbility(EIJPAbilitySlot::ClassSkill);

	Mode->SetRival(IJPRivalSkillTests::MakeSnareRival());
	UIJPAbilityComponent* Abilities = RivalPaddle->GetAbilities();
	UTEST_TRUE("The rival's skill replaces the class's", Abilities->GetAbility(EIJPAbilitySlot::ClassSkill)->IsA<UIJPAbility_Snare>());
	RivalPaddle->GetController()->UnPossess(); // we press the button ourselves
	const float NormalSpeed = Player->GetMaxSpeed();

	// The telegraph: a glow and a warning buzz, and nothing happens yet.
	UTEST_TRUE("Triggered", Abilities->TryActivate(EIJPAbilitySlot::ClassSkill));
	UTEST_TRUE("Winding up", Abilities->IsWindingUp(EIJPAbilitySlot::ClassSkill));
	UTEST_TRUE("Warning tone", Arena->GetTones()->GetLastTone() == Arena->GetToneSet().Warn);
	Test.Step();
	UTEST_TRUE("The rival glows", RivalPaddle->IsArmedCueShown());
	UTEST_EQUAL("Not slowed yet", Player->GetMaxSpeed(), NormalSpeed);
	UTEST_FALSE("Can't trigger again", Abilities->TryActivate(EIJPAbilitySlot::ClassSkill));

	// Then it lands: slower, and a dash covers less ground.
	Test.RunFor(0.5f);
	UTEST_FALSE("Wound up", Abilities->IsWindingUp(EIJPAbilitySlot::ClassSkill));
	UTEST_EQUAL_TOLERANCE("Half speed", Player->GetMaxSpeed(), NormalSpeed * 0.5f, 0.01f);
	UTEST_FALSE("The glow is gone", RivalPaddle->IsArmedCueShown());
	const float StartY = Player->GetPlanePosition().Y;
	Player->Dash(60.f, 0.1f);
	Test.RunFor(0.3f);
	const float Dashed = FMath::Abs(Player->GetPlanePosition().Y - StartY);
	UTEST_TRUE(*FString::Printf(TEXT("A shorter dash (%.1f of 60)"), Dashed), Dashed > 20.f && Dashed < 45.f);

	// And wears off.
	Test.RunFor(0.8f);
	UTEST_EQUAL("Back to full speed", Player->GetMaxSpeed(), NormalSpeed);

	// A rival with no skill of its own keeps the class's.
	Mode->SetRival(NewObject<UIJPRival>(GetTransientPackage()));
	UTEST_EQUAL("Class skill back", Abilities->GetAbility(EIJPAbilitySlot::ClassSkill)->GetClass(), ClassSkill->GetClass());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPRivalAIUsesSkillTest, "IJPong.RivalSkill.AIUsesItWhenItsMoment", IJPRivalSkillTests::Flags)
bool FIJPRivalAIUsesSkillTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test;
	AIJPTestGameMode* Mode = IJPRivalSkillTests::GetMode(Test);
	AIJPArena* Arena = Mode->GetArena();
	AIJPPaddle* Player = Arena->GetPaddle(EIJPSide::Left);
	Mode->SetRival(IJPRivalSkillTests::MakeSnareRival());
	UIJPAbilityComponent* Abilities = Arena->GetPaddle(EIJPSide::Right)->GetAbilities();

	// Waiting for the serve: no ball heading your way, so the rival holds it.
	Test.RunFor(0.3f);
	UTEST_EQUAL("Not used yet", Abilities->GetCooldownRemaining(EIJPAbilitySlot::ClassSkill), 0.f);

	// A ball on its way to you: the rival snares you.
	Arena->GetBall()->Serve(EIJPSide::Left, 0.f);
	UTEST_TRUE("The rival triggers it", IJPRivalSkillTests::RunUntil(Test, 0.3f, [Abilities] { return Abilities->IsWindingUp(EIJPAbilitySlot::ClassSkill); }));
	UTEST_TRUE("It lands", IJPRivalSkillTests::RunUntil(Test, 1.f, [Player] { return Player->GetSpeedScale() < 1.f; }));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPBreakerTest, "IJPong.RivalSkill.BreakerCracksOneShotThroughABarrier", IJPRivalSkillTests::Flags)
bool FIJPBreakerTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test;
	AIJPTestGameMode* Mode = IJPRivalSkillTests::GetMode(Test);
	AIJPArena* Arena = Mode->GetArena();
	AIJPPaddle* Player = Arena->GetPaddle(EIJPSide::Left);
	AIJPPaddle* RivalPaddle = Arena->GetPaddle(EIJPSide::Right);
	UIJPMatchComponent* Match = Mode->GetMatch();
	UIJPRival* Rival = NewObject<UIJPRival>(GetTransientPackage());
	Rival->RivalSkill = NewObject<UIJPAbility_Breaker>(Rival);
	Mode->SetRival(Rival);
	RivalPaddle->GetController()->UnPossess(); // it stays in the middle and returns straight
	UIJPAbilityComponent* Abilities = RivalPaddle->GetAbilities();

	// The player hides at the top behind a barrier.
	const auto HoldUp = [Player] { Player->AddMoveInput(1.f); };
	Test.RunFor(0.5f, HoldUp);
	Arena->SetBarrierUp(EIJPSide::Left, true);

	// Armed after the wind-up; the rival's next return cracks through.
	UTEST_TRUE("Triggered", Abilities->TryActivate(EIJPAbilitySlot::ClassSkill));
	Test.RunFor(0.6f, HoldUp);
	UTEST_TRUE("Armed", Abilities->IsArmed());
	AIJPBall* Ball = Arena->GetBall();
	Ball->Serve(EIJPSide::Right, 0.f);
	UTEST_TRUE("Returned", IJPRivalSkillTests::RunUntil(Test, 2.f, [Ball] { return Ball->GetPlaneVelocity().X < 0.f; }));
	UTEST_TRUE("That ball pierces", Ball->IsPiercing());
	UTEST_FALSE("Spent", Abilities->IsArmed());
	const int32 GoalsBefore = Match->GetGoals(EIJPSide::Right);
	UTEST_TRUE("Through the barrier and in", IJPRivalSkillTests::RunUntil(Test, 2.f, [Ball] { return !Ball->IsInPlay(); }));
	UTEST_EQUAL("A goal", Match->GetGoals(EIJPSide::Right), GoalsBefore + 1);
	UTEST_TRUE("The barrier is still up", Arena->IsBarrierUp(EIJPSide::Left));

	// The next ordinary return is stopped as usual.
	Test.RunFor(0.1f, HoldUp);
	Ball->Serve(EIJPSide::Right, 0.f);
	UTEST_TRUE("Returned again", IJPRivalSkillTests::RunUntil(Test, 2.f, [Ball] { return Ball->GetPlaneVelocity().X < 0.f; }));
	UTEST_FALSE("Not piercing", Ball->IsPiercing());
	UTEST_TRUE("Bounced off the barrier", IJPRivalSkillTests::RunUntil(Test, 2.f, [Ball] { return Ball->GetPlaneVelocity().X > 0.f; }));
	UTEST_TRUE("Still in play", Ball->IsInPlay());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPBreakerAITest, "IJPong.RivalSkill.BreakerOnlyArmsAgainstABarrier", IJPRivalSkillTests::Flags)
bool FIJPBreakerAITest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test;
	AIJPTestGameMode* Mode = IJPRivalSkillTests::GetMode(Test);
	AIJPArena* Arena = Mode->GetArena();
	UIJPRival* Rival = NewObject<UIJPRival>(GetTransientPackage());
	Rival->RivalSkill = NewObject<UIJPAbility_Breaker>(Rival);
	Mode->SetRival(Rival);
	UIJPAbilityComponent* Abilities = Arena->GetPaddle(EIJPSide::Right)->GetAbilities();
	AIJPBall* Ball = Arena->GetBall();

	// A Classic player has no barrier: a ball on its way to the rival isn't worth arming for.
	Ball->Serve(EIJPSide::Right, 0.f);
	Test.RunFor(0.3f);
	UTEST_EQUAL("Held back", Abilities->GetCooldownRemaining(EIJPAbilitySlot::ClassSkill), 0.f);

	// With a barrier up, it arms.
	Arena->SetBarrierUp(EIJPSide::Left, true);
	Ball->Serve(EIJPSide::Right, 0.f);
	UTEST_TRUE("Arms", IJPRivalSkillTests::RunUntil(Test, 0.3f, [Abilities] { return Abilities->IsWindingUp(EIJPAbilitySlot::ClassSkill); }));
	return true;
}

#endif
