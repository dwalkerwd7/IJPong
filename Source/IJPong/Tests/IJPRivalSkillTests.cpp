// It's Just Pong

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "AI/IJPPaddleAIController.h"
#include "Abilities/IJPAbility_Breaker.h"
#include "Abilities/IJPAbility_Catch.h"
#include "Abilities/IJPAbility_Glutton.h"
#include "Abilities/IJPAbility_Grow.h"
#include "Abilities/IJPAbility_Jammer.h"
#include "Abilities/IJPAbility_Mirror.h"
#include "Abilities/IJPAbility_Reader.h"
#include "Abilities/IJPAbility_Scorcher.h"
#include "Abilities/IJPAbility_Smash.h"
#include "Abilities/IJPAbility_Magnet.h"
#include "Abilities/IJPAbility_Snare.h"
#include "Abilities/IJPAbility_Warp.h"
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
#include "Presentation/IJPCRTComponent.h"
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPMagnetTest, "IJPong.RivalSkill.MagnetSoftensSmashesAndCurves", IJPRivalSkillTests::Flags)
bool FIJPMagnetTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test;
	AIJPTestGameMode* Mode = IJPRivalSkillTests::GetMode(Test);
	AIJPArena* Arena = Mode->GetArena();
	UIJPRival* Rival = NewObject<UIJPRival>(GetTransientPackage());
	UIJPAbility_Magnet* Magnet = NewObject<UIJPAbility_Magnet>(Rival);
	Magnet->Telegraph = 0.1f;
	Rival->RivalSkill = Magnet;
	Mode->SetRival(Rival);
	AIJPPaddle* RivalPaddle = Arena->GetPaddle(EIJPSide::Right);
	RivalPaddle->GetController()->UnPossess();
	UIJPAbilityComponent* Abilities = RivalPaddle->GetAbilities();

	// A smashed, curving shot on its way to the rival.
	AIJPBall* Ball = Arena->GetBall();
	Ball->Serve(EIJPSide::Right, 0.f);
	const float BaseSpeed = Ball->GetPlaneVelocity().Size();
	Ball->Boost(2.f);
	Ball->Curve(60.f, 2.f, 1.f);
	const float SmashSpeed = Ball->GetPlaneVelocity().Size();

	UTEST_TRUE("Triggered", Abilities->TryActivate(EIJPAbilitySlot::ClassSkill));
	UTEST_EQUAL("Nothing during the wind-up", Ball->GetCurveRate(), 60.f);
	Test.RunFor(0.15f);
	UTEST_EQUAL_TOLERANCE("Half the extra speed gone", static_cast<float>(Ball->GetPlaneVelocity().Size()), (BaseSpeed + SmashSpeed) * 0.5f, 1.f);
	UTEST_EQUAL_TOLERANCE("The curve bends half as fast", Ball->GetCurveRate(), 30.f, 0.01f);
	UTEST_TRUE("Still a smash, still a curve: softened, not cancelled", Ball->IsBoosted() && Ball->IsCurving());

	// Once per ball: the rest of the window doesn't keep eating at it.
	Test.RunFor(0.3f);
	UTEST_EQUAL_TOLERANCE("Pulled only once", Ball->GetCurveRate(), 30.f, 0.01f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPMagnetAITest, "IJPong.RivalSkill.MagnetWaitsForATrickShot", IJPRivalSkillTests::Flags)
bool FIJPMagnetAITest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test;
	AIJPTestGameMode* Mode = IJPRivalSkillTests::GetMode(Test);
	AIJPArena* Arena = Mode->GetArena();
	UIJPRival* Rival = NewObject<UIJPRival>(GetTransientPackage());
	Rival->RivalSkill = NewObject<UIJPAbility_Magnet>(Rival);
	Mode->SetRival(Rival);
	UIJPAbilityComponent* Abilities = Arena->GetPaddle(EIJPSide::Right)->GetAbilities();
	AIJPBall* Ball = Arena->GetBall();

	// A plain ball: not worth it.
	Ball->Serve(EIJPSide::Right, 0.f);
	Test.RunFor(0.2f);
	UTEST_EQUAL("Held back", Abilities->GetCooldownRemaining(EIJPAbilitySlot::ClassSkill), 0.f);

	// A smash: pull it in.
	Ball->Serve(EIJPSide::Right, 0.f);
	Ball->Boost(1.6f);
	UTEST_TRUE("Uses it", IJPRivalSkillTests::RunUntil(Test, 0.2f, [Abilities] { return Abilities->IsWindingUp(EIJPAbilitySlot::ClassSkill); }));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPGluttonTest, "IJPong.RivalSkill.GluttonSwallowsOneExtraBall", IJPRivalSkillTests::Flags)
bool FIJPGluttonTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test;
	AIJPTestGameMode* Mode = IJPRivalSkillTests::GetMode(Test);
	AIJPArena* Arena = Mode->GetArena();
	UIJPMatchComponent* Match = Mode->GetMatch();
	UIJPRival* Rival = NewObject<UIJPRival>(GetTransientPackage());
	UIJPAbility_Glutton* Glutton = NewObject<UIJPAbility_Glutton>(Rival);
	Glutton->Telegraph = 0.1f;
	Rival->RivalSkill = Glutton;
	Mode->SetRival(Rival);
	AIJPPaddle* RivalPaddle = Arena->GetPaddle(EIJPSide::Right);
	RivalPaddle->GetController()->UnPossess();
	UIJPAbilityComponent* Abilities = RivalPaddle->GetAbilities();
	const UIJPAbility_Glutton* Equipped = Cast<UIJPAbility_Glutton>(Abilities->GetAbility(EIJPAbilitySlot::ClassSkill));

	// One ball: nothing to eat, even with its mouth open.
	AIJPBall* Main = Arena->GetBall();
	Main->Serve(EIJPSide::Right, 0.f);
	UTEST_TRUE("Triggered", Abilities->TryActivate(EIJPAbilitySlot::ClassSkill));
	Test.RunFor(0.3f);
	UTEST_TRUE("Mouth open", Abilities->IsArmed());
	UTEST_TRUE("A lone ball is safe", Main->IsInPlay());

	// A second ball: one of them is swallowed, without a goal.
	AIJPBall* Extra = Arena->AddBall(nullptr);
	Extra->Serve(EIJPSide::Right, 20.f);
	const float RivalHealth = Match->GetHealth(EIJPSide::Right);
	Test.Step();
	UTEST_EQUAL("Swallowed one", Equipped->GetSwallowed(), 1);
	UTEST_EQUAL("The other plays on", Arena->GetNumBallsInPlay(), 1);
	UTEST_EQUAL("No goal for it", Match->GetHealth(EIJPSide::Right), RivalHealth);
	UTEST_FALSE("Mouth shut", Abilities->IsArmed());

	// One per use: another pair gets through whole.
	Extra = Arena->AddBall(nullptr);
	Extra->Serve(EIJPSide::Right, -20.f);
	Test.RunFor(0.2f);
	UTEST_EQUAL("Still only one", Equipped->GetSwallowed(), 1);
	UTEST_EQUAL("Both in play", Arena->GetNumBallsInPlay(), 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPGluttonAITest, "IJPong.RivalSkill.GluttonWaitsForMultiBall", IJPRivalSkillTests::Flags)
bool FIJPGluttonAITest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test;
	AIJPTestGameMode* Mode = IJPRivalSkillTests::GetMode(Test);
	AIJPArena* Arena = Mode->GetArena();
	UIJPRival* Rival = NewObject<UIJPRival>(GetTransientPackage());
	Rival->RivalSkill = NewObject<UIJPAbility_Glutton>(Rival);
	Mode->SetRival(Rival);
	UIJPAbilityComponent* Abilities = Arena->GetPaddle(EIJPSide::Right)->GetAbilities();

	Arena->GetBall()->Serve(EIJPSide::Right, 0.f);
	Test.RunFor(0.2f);
	UTEST_EQUAL("One ball: held back", Abilities->GetCooldownRemaining(EIJPAbilitySlot::ClassSkill), 0.f);

	Arena->AddBall(nullptr)->Serve(EIJPSide::Right, 20.f);
	UTEST_TRUE("Two: opens up", IJPRivalSkillTests::RunUntil(Test, 0.2f, [Abilities] { return Abilities->IsWindingUp(EIJPAbilitySlot::ClassSkill); }));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPMirrorTest, "IJPong.RivalSkill.MirrorFiresYourLastSkillBack", IJPRivalSkillTests::Flags)
bool FIJPMirrorTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test;
	AIJPTestGameMode* Mode = IJPRivalSkillTests::GetMode(Test);
	AIJPArena* Arena = Mode->GetArena();
	AIJPPaddle* Player = Arena->GetPaddle(EIJPSide::Left);
	AIJPPaddle* RivalPaddle = Arena->GetPaddle(EIJPSide::Right);
	UIJPRival* Rival = NewObject<UIJPRival>(GetTransientPackage());
	UIJPAbility_Mirror* Mirror = NewObject<UIJPAbility_Mirror>(Rival);
	Mirror->Telegraph = 0.1f;
	Mirror->Cooldown = 0.5f;
	Rival->RivalSkill = Mirror;
	Mode->SetRival(Rival);
	RivalPaddle->GetController()->UnPossess(); // stays in the middle, returns straight
	UIJPAbilityComponent* Abilities = RivalPaddle->GetAbilities();
	const UIJPAbility_Mirror* Equipped = Cast<UIJPAbility_Mirror>(Abilities->GetAbility(EIJPAbilitySlot::ClassSkill));
	UIJPAbilityComponent* PlayerAbilities = Player->GetAbilities();
	PlayerAbilities->Equip(EIJPAbilitySlot::RunAbility, NewObject<UIJPAbility_Grow>(GetTransientPackage()));

	UTEST_FALSE("Nothing to copy yet", Abilities->TryActivate(EIJPAbilitySlot::ClassSkill));

	// The player smashes (Classic's class skill): the rival copies it and smashes back.
	UTEST_TRUE("Player smashes", PlayerAbilities->TryActivate(EIJPAbilitySlot::ClassSkill));
	UTEST_TRUE("Seen", Equipped->GetLastSeen() && Equipped->GetLastSeen()->IsA<UIJPAbility_Smash>());
	UTEST_TRUE("Mirrors it", Abilities->TryActivate(EIJPAbilitySlot::ClassSkill));
	Test.RunFor(0.15f);
	UTEST_TRUE("Its own smash, armed", Equipped->GetMirrored() && Equipped->GetMirrored()->IsA<UIJPAbility_Smash>() && Abilities->IsArmed());
	UTEST_TRUE("With the armed glow", RivalPaddle->IsArmedCueShown());
	AIJPBall* Ball = Arena->GetBall();
	Ball->Serve(EIJPSide::Right, 0.f);
	UTEST_TRUE("Returned", IJPRivalSkillTests::RunUntil(Test, 2.f, [Ball] { return Ball->GetPlaneVelocity().X < 0.f; }));
	UTEST_TRUE("Smashed back at you", Ball->IsBoosted());
	UTEST_FALSE("Spent", Abilities->IsArmed());

	// The latest skill wins: the player grows, so the rival grows.
	UTEST_TRUE("Player grows", PlayerAbilities->TryActivate(EIJPAbilitySlot::RunAbility));
	const float NormalLength = RivalPaddle->GetSize().Y;
	Test.RunFor(0.6f); // the mirror's cooldown
	UTEST_TRUE("Mirrors again", Abilities->TryActivate(EIJPAbilitySlot::ClassSkill));
	Test.RunFor(0.15f);
	UTEST_EQUAL_TOLERANCE("The rival grew", static_cast<float>(RivalPaddle->GetSize().Y), static_cast<float>(NormalLength) * 2.f, 0.01f);
	UTEST_EQUAL_TOLERANCE("The player too, from their own", static_cast<float>(Player->GetSize().Y), static_cast<float>(NormalLength) * 2.f, 0.01f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPJammerTest, "IJPong.RivalSkill.JammerLocksYourSkillBehindStatic", IJPRivalSkillTests::Flags)
bool FIJPJammerTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test;
	AIJPTestGameMode* Mode = IJPRivalSkillTests::GetMode(Test);
	AIJPArena* Arena = Mode->GetArena();
	AIJPPaddle* Player = Arena->GetPaddle(EIJPSide::Left);
	AIJPPaddle* RivalPaddle = Arena->GetPaddle(EIJPSide::Right);
	UIJPRival* Rival = NewObject<UIJPRival>(GetTransientPackage());
	UIJPAbility_Jammer* Jammer = NewObject<UIJPAbility_Jammer>(Rival);
	Jammer->Telegraph = 0.1f;
	Jammer->Duration = 1.f;
	Rival->RivalSkill = Jammer;
	Mode->SetRival(Rival);
	RivalPaddle->GetController()->UnPossess();
	UIJPAbilityComponent* PlayerAbilities = Player->GetAbilities();
	PlayerAbilities->Equip(EIJPAbilitySlot::RunAbility, NewObject<UIJPAbility_Grow>(GetTransientPackage()));
	UIJPCRTComponent* CRT = Arena->GetCRT();

	UTEST_TRUE("Jams", RivalPaddle->GetAbilities()->TryActivate(EIJPAbilitySlot::ClassSkill));
	Test.RunFor(0.15f);
	UTEST_TRUE("Your class skill is locked", PlayerAbilities->IsLocked(EIJPAbilitySlot::ClassSkill));
	UTEST_FALSE("Pressing it does nothing", PlayerAbilities->TryActivate(EIJPAbilitySlot::ClassSkill));
	UTEST_TRUE("Static on the screen", CRT->GetJam() > 0.f);
	UTEST_TRUE("Your run ability still works", PlayerAbilities->TryActivate(EIJPAbilitySlot::RunAbility));

	// It clears.
	Test.RunFor(1.f);
	UTEST_FALSE("Unlocked", PlayerAbilities->IsLocked(EIJPAbilitySlot::ClassSkill));
	UTEST_EQUAL("The static's gone", CRT->GetJam(), 0.f);
	UTEST_TRUE("Your skill is back", PlayerAbilities->TryActivate(EIJPAbilitySlot::ClassSkill));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPJammerAITest, "IJPong.RivalSkill.JammerWaitsUntilYourSkillIsReady", IJPRivalSkillTests::Flags)
bool FIJPJammerAITest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test;
	AIJPTestGameMode* Mode = IJPRivalSkillTests::GetMode(Test);
	AIJPArena* Arena = Mode->GetArena();
	UIJPRival* Rival = NewObject<UIJPRival>(GetTransientPackage());
	Rival->RivalSkill = NewObject<UIJPAbility_Jammer>(Rival);
	Mode->SetRival(Rival);
	UIJPAbilityComponent* Abilities = Arena->GetPaddle(EIJPSide::Right)->GetAbilities();
	UIJPAbilityComponent* PlayerAbilities = Arena->GetPaddle(EIJPSide::Left)->GetAbilities();
	AIJPBall* Ball = Arena->GetBall();

	// Your skill already spent: nothing to jam.
	PlayerAbilities->TryActivate(EIJPAbilitySlot::ClassSkill);
	Ball->Serve(EIJPSide::Left, 0.f);
	Test.RunFor(0.2f);
	UTEST_EQUAL("Held back", Abilities->GetCooldownRemaining(EIJPAbilitySlot::ClassSkill), 0.f);

	// Ready again, ball coming at you: jammed.
	PlayerAbilities->Equip(EIJPAbilitySlot::ClassSkill, NewObject<UIJPAbility_Grow>(GetTransientPackage()));
	UTEST_TRUE("Jams", IJPRivalSkillTests::RunUntil(Test, 0.2f, [Abilities] { return Abilities->IsWindingUp(EIJPAbilitySlot::ClassSkill); }));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPWarpTest, "IJPong.RivalSkill.WarpMirrorsTheBallsHeight", IJPRivalSkillTests::Flags)
bool FIJPWarpTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test;
	AIJPTestGameMode* Mode = IJPRivalSkillTests::GetMode(Test);
	AIJPArena* Arena = Mode->GetArena();
	AIJPPaddle* RivalPaddle = Arena->GetPaddle(EIJPSide::Right);
	UIJPRival* Rival = NewObject<UIJPRival>(GetTransientPackage());
	UIJPAbility_Warp* Warp = NewObject<UIJPAbility_Warp>(Rival);
	Warp->Telegraph = 0.1f;
	Warp->Cooldown = 0.2f;
	Warp->MinJump = 120.f;
	Rival->RivalSkill = Warp;
	Mode->SetRival(Rival);
	RivalPaddle->GetController()->UnPossess();
	UIJPAbilityComponent* Abilities = RivalPaddle->GetAbilities();
	AIJPBall* Ball = Arena->GetBall();

	// High on the way to the player: it reappears as far below, same speed and heading.
	Ball->Launch(FVector2D(-50.f, 100.f), FVector2D(-300.f, 0.f));
	Ball->Boost(1.5f);
	UTEST_TRUE("Triggered", Abilities->TryActivate(EIJPAbilitySlot::ClassSkill));
	UTEST_TRUE("Warped", IJPRivalSkillTests::RunUntil(Test, 0.3f, [Ball] { return Ball->GetPlanePosition().Y < 0.f; }));
	UTEST_EQUAL_TOLERANCE("Mirrored height", static_cast<float>(Ball->GetPlanePosition().Y), -100.f, 0.5f);
	UTEST_EQUAL_TOLERANCE("Same velocity", static_cast<float>(Ball->GetPlaneVelocity().X), -450.f, 0.5f);
	UTEST_TRUE("Still the smash it was", Ball->IsBoosted() && Ball->IsInPlay());

	// Near the middle, mirroring would barely move it: it jumps at least MinJump.
	Test.RunFor(0.2f);
	Ball->Launch(FVector2D(-50.f, 10.f), FVector2D(-300.f, 0.f));
	UTEST_TRUE("Triggered again", Abilities->TryActivate(EIJPAbilitySlot::ClassSkill));
	UTEST_TRUE("Warped again", IJPRivalSkillTests::RunUntil(Test, 0.3f, [Ball] { return Ball->GetPlanePosition().Y < 0.f; }));
	UTEST_EQUAL_TOLERANCE("A real jump", static_cast<float>(Ball->GetPlanePosition().Y), -110.f, 0.5f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPWarpAITest, "IJPong.RivalSkill.WarpFiresAsTheBallCrossesTheNet", IJPRivalSkillTests::Flags)
bool FIJPWarpAITest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test;
	AIJPTestGameMode* Mode = IJPRivalSkillTests::GetMode(Test);
	AIJPArena* Arena = Mode->GetArena();
	UIJPRival* Rival = NewObject<UIJPRival>(GetTransientPackage());
	Rival->RivalSkill = NewObject<UIJPAbility_Warp>(Rival);
	Mode->SetRival(Rival);
	UIJPAbilityComponent* Abilities = Arena->GetPaddle(EIJPSide::Right)->GetAbilities();
	AIJPBall* Ball = Arena->GetBall();

	// Coming at the rival: not a warp moment.
	Ball->Launch(FVector2D(-30.f, 0.f), FVector2D(300.f, 0.f));
	Test.RunFor(0.3f);
	UTEST_EQUAL("Held back", Abilities->GetCooldownRemaining(EIJPAbilitySlot::ClassSkill), 0.f);

	// Crossing the net toward the player: warp it.
	Ball->Launch(FVector2D(0.f, 50.f), FVector2D(-300.f, 0.f));
	UTEST_TRUE("Warps", IJPRivalSkillTests::RunUntil(Test, 0.3f, [Abilities] { return Abilities->IsWindingUp(EIJPAbilitySlot::ClassSkill); }));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPReaderTest, "IJPong.RivalSkill.ReaderMovesToYourAimBeforeYouFire", IJPRivalSkillTests::Flags)
bool FIJPReaderTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test;
	AIJPTestGameMode* Mode = IJPRivalSkillTests::GetMode(Test);
	AIJPArena* Arena = Mode->GetArena();
	AIJPPaddle* Player = Arena->GetPaddle(EIJPSide::Left);
	AIJPPaddle* RivalPaddle = Arena->GetPaddle(EIJPSide::Right);
	UIJPRival* Rival = NewObject<UIJPRival>(GetTransientPackage());
	UIJPAbility_Reader* Reader = NewObject<UIJPAbility_Reader>(Rival);
	Reader->Telegraph = 0.1f;
	Rival->RivalSkill = Reader;
	Mode->SetRival(Rival);
	UIJPAbilityComponent* Abilities = RivalPaddle->GetAbilities();

	// The player catches with a long hold.
	UIJPAbility_Catch* Catch = NewObject<UIJPAbility_Catch>(GetTransientPackage());
	Catch->HoldTime = 3.f;
	Player->GetAbilities()->Equip(EIJPAbilitySlot::ClassSkill, Catch);
	Player->GetAbilities()->TryActivate(EIJPAbilitySlot::ClassSkill);
	UTEST_TRUE("The rival starts reading as soon as you arm", IJPRivalSkillTests::RunUntil(Test, 0.2f, [Abilities] { return Abilities->IsWindingUp(EIJPAbilitySlot::ClassSkill); }));
	AIJPBall* Ball = Arena->GetBall();
	Ball->Serve(EIJPSide::Left, 0.f);
	UTEST_TRUE("Caught", IJPRivalSkillTests::RunUntil(Test, 2.f, [Ball] { return Ball->IsHeld(); }));

	// Aim up: the rival heads for where that shot will land, before it's fired.
	Player->SetAimAngle(35.f);
	const UIJPAbility_Reader* Equipped = Cast<UIJPAbility_Reader>(Abilities->GetAbility(EIJPAbilitySlot::ClassSkill));
	float Predicted = 0.f;
	UTEST_TRUE("Reads the shot", Equipped->PredictHeldShot(Predicted));
	UTEST_TRUE(*FString::Printf(TEXT("Somewhere worth moving to (%.0f)"), Predicted), FMath::Abs(Predicted) > 40.f);
	Test.RunFor(1.2f);
	UTEST_TRUE("Still holding", Ball->IsHeld());
	UTEST_EQUAL_TOLERANCE("Already there", static_cast<float>(RivalPaddle->GetPlanePosition().Y), Predicted, 15.f);

	// Once it's fired, the rival plays the ball as normal again.
	Player->GetAbilities()->Release(EIJPAbilitySlot::ClassSkill);
	Test.Step();
	UTEST_FALSE("Reading stops", Cast<AIJPPaddleAIController>(RivalPaddle->GetController())->HasReadTarget());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPScorcherTest, "IJPong.RivalSkill.ScorcherBurnsYourHoldTime", IJPRivalSkillTests::Flags)
bool FIJPScorcherTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test;
	AIJPTestGameMode* Mode = IJPRivalSkillTests::GetMode(Test);
	AIJPArena* Arena = Mode->GetArena();
	AIJPPaddle* Player = Arena->GetPaddle(EIJPSide::Left);
	AIJPPaddle* RivalPaddle = Arena->GetPaddle(EIJPSide::Right);
	UIJPRival* Rival = NewObject<UIJPRival>(GetTransientPackage());
	UIJPAbility_Scorcher* Scorcher = NewObject<UIJPAbility_Scorcher>(Rival);
	Scorcher->Telegraph = 0.1f;
	Rival->RivalSkill = Scorcher;
	Mode->SetRival(Rival);
	RivalPaddle->GetController()->UnPossess(); // stays in the middle, returns straight
	UIJPAbility_Catch* Catch = NewObject<UIJPAbility_Catch>(GetTransientPackage());
	Catch->HoldTime = 1.f;
	Player->GetAbilities()->Equip(EIJPAbilitySlot::ClassSkill, Catch);

	// The rival heats up, then returns a ball: it goes back hot.
	UTEST_TRUE("Heats up", RivalPaddle->GetAbilities()->TryActivate(EIJPAbilitySlot::ClassSkill));
	Test.RunFor(0.15f);
	UTEST_TRUE("Glowing", RivalPaddle->GetAbilities()->IsArmed());
	Player->GetAbilities()->TryActivate(EIJPAbilitySlot::ClassSkill);
	AIJPBall* Ball = Arena->GetBall();
	Ball->Serve(EIJPSide::Right, 0.f);

	// Caught hot: the hold burns down to 35% of a second.
	UTEST_TRUE("Caught", IJPRivalSkillTests::RunUntil(Test, 3.f, [Ball] { return Ball->IsHeld(); }));
	UTEST_EQUAL_TOLERANCE("It arrived hot", Ball->GetArrivalHeat(), 0.65f, 0.01f);
	Test.RunFor(0.3f);
	UTEST_TRUE("Still in hand just before", Ball->IsHeld());
	Test.RunFor(0.1f);
	UTEST_FALSE("Fired early: too hot to hold", Ball->IsHeld());
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
