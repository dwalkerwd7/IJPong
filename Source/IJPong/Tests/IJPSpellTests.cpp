// It's Just Pong

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Abilities/IJPAbility_Spell.h"
#include "Abilities/IJPAbilityComponent.h"
#include "Core/IJPTestGameMode.h"
#include "Core/IJPTypes.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "Gameplay/IJPArena.h"
#include "Gameplay/IJPBall.h"
#include "Gameplay/IJPChargePipsComponent.h"
#include "Gameplay/IJPMatchComponent.h"
#include "Gameplay/IJPPaddle.h"
#include "Gameplay/IJPRival.h"
#include "Gameplay/IJPSpellStrike.h"
#include "Tests/IJPTestWorld.h"

namespace IJPSpellTests
{
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter;

	AIJPTestGameMode* GetMode(const FIJPTestWorld& Test)
	{
		return Cast<AIJPTestGameMode>(Test.GetWorld()->GetAuthGameMode());
	}

	UIJPAbility_Spell* MakeSpell(int32 Cost, float Delay, float Damage, bool bTravels)
	{
		UIJPAbility_Spell* Spell = NewObject<UIJPAbility_Spell>(GetTransientPackage());
		Spell->ChargeCost = Cost;
		Spell->Strike.Delay = Delay;
		Spell->Strike.Damage = Damage;
		Spell->Strike.HalfHeight = 20.f;
		Spell->Strike.bTravels = bTravels;
		return Spell;
	}

	/** Pretend the paddle returned the ball Count times. */
	void Charge(AIJPArena* Arena, UIJPAbilityComponent* Abilities, int32 Count)
	{
		for (int32 i = 0; i < Count; ++i)
		{
			Abilities->HandleBallHit(*Arena->GetBall());
		}
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPSpellChargeTest, "IJPong.Spell.ChargedByReturnsShownAsPips", IJPSpellTests::Flags)
bool FIJPSpellChargeTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test;
	AIJPArena* Arena = Test.GetArena();
	UIJPAbilityComponent* Abilities = Arena->GetPaddle(EIJPSide::Left)->GetAbilities();
	UIJPChargePipsComponent* Pips = Arena->GetChargePips(EIJPSide::Left);
	UTEST_EQUAL("No spell, no pips", Pips->GetTotal(), 0);

	Abilities->Equip(EIJPAbilitySlot::Spell, IJPSpellTests::MakeSpell(3, 0.3f, 1.f, false));
	UTEST_EQUAL("Empty pips", Pips->GetTotal(), 3);
	UTEST_FALSE("Not charged", Abilities->TryActivate(EIJPAbilitySlot::Spell));

	IJPSpellTests::Charge(Arena, Abilities, 2);
	UTEST_EQUAL("Two lit", Pips->GetFilled(), 2);
	UTEST_FALSE("Still short", Abilities->IsReady(EIJPAbilitySlot::Spell));
	IJPSpellTests::Charge(Arena, Abilities, 5);
	UTEST_EQUAL("Capped at full", Abilities->GetCharge(EIJPAbilitySlot::Spell), 3);
	UTEST_TRUE("Ready blinks", Pips->IsBlinking());

	UTEST_TRUE("Cast", Abilities->TryActivate(EIJPAbilitySlot::Spell));
	UTEST_EQUAL("Spent", Pips->GetFilled(), 0);
	UTEST_FALSE("Stops blinking", Pips->IsBlinking());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPSpellHitTest, "IJPong.Spell.LandsOnAStillPaddleAndIsDodgedByMoving", IJPSpellTests::Flags)
bool FIJPSpellHitTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test;
	AIJPTestGameMode* Mode = IJPSpellTests::GetMode(Test);
	AIJPArena* Arena = Mode->GetArena();
	UIJPMatchComponent* Match = Mode->GetMatch();
	AIJPPaddle* Rival = Arena->GetPaddle(EIJPSide::Right);
	Rival->GetController()->UnPossess();
	UIJPAbilityComponent* Abilities = Arena->GetPaddle(EIJPSide::Left)->GetAbilities();

	// Lightning at a paddle that stays put: a hit, and never a goal.
	Abilities->Equip(EIJPAbilitySlot::Spell, IJPSpellTests::MakeSpell(1, 0.3f, 0.5f, false));
	IJPSpellTests::Charge(Arena, Abilities, 1);
	UTEST_TRUE("Cast", Abilities->TryActivate(EIJPAbilitySlot::Spell));
	const AIJPSpellStrike* Bolt = Cast<UIJPAbility_Spell>(Abilities->GetAbility(EIJPAbilitySlot::Spell))->GetLastStrike();
	UTEST_NOT_NULL("On its way", Bolt);
	UTEST_EQUAL("At the rival's lane", Bolt->GetTargetSide(), EIJPSide::Right);
	UTEST_EQUAL("Where it stood", Bolt->GetTargetY(), static_cast<float>(Rival->GetPlanePosition().Y));
	UTEST_EQUAL("Warned about", Arena->GetStrikes().Num(), 1);
	Test.RunFor(0.35f);
	UTEST_EQUAL_TOLERANCE("Hit", Match->GetHealth(EIJPSide::Right), 4.5f, 0.001f);
	UTEST_EQUAL("Not a goal", Match->GetGoals(EIJPSide::Left), 0);

	// A fireball at a paddle that moves off: dodged.
	Abilities->Equip(EIJPAbilitySlot::Spell, IJPSpellTests::MakeSpell(1, 1.f, 1.5f, true));
	IJPSpellTests::Charge(Arena, Abilities, 1);
	UTEST_TRUE("Cast again", Abilities->TryActivate(EIJPAbilitySlot::Spell));
	Test.RunFor(1.1f, [Rival] { Rival->AddMoveInput(1.f); });
	UTEST_EQUAL_TOLERANCE("Dodged", Match->GetHealth(EIJPSide::Right), 4.5f, 0.001f);
	Test.RunFor(0.3f);
	UTEST_EQUAL("Cleaned up", Arena->GetStrikes().Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPSpellAITest, "IJPong.Spell.AIDodgesAndRivalsCast", IJPSpellTests::Flags)
bool FIJPSpellAITest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test;
	AIJPTestGameMode* Mode = IJPSpellTests::GetMode(Test);
	AIJPArena* Arena = Mode->GetArena();
	UIJPMatchComponent* Match = Mode->GetMatch();

	// No ball to chase: the AI steps out of a slow fireball's zone.
	UIJPAbilityComponent* Abilities = Arena->GetPaddle(EIJPSide::Left)->GetAbilities();
	Abilities->Equip(EIJPAbilitySlot::Spell, IJPSpellTests::MakeSpell(1, 1.2f, 1.5f, true));
	IJPSpellTests::Charge(Arena, Abilities, 1);
	Abilities->TryActivate(EIJPAbilitySlot::Spell);
	Test.RunFor(1.4f);
	UTEST_EQUAL("The AI dodged", Match->GetHealth(EIJPSide::Right), 5.f);

	// A rival with a spell casts it once charged, as a ball heads for the player.
	UIJPRival* Rival = NewObject<UIJPRival>(GetTransientPackage());
	Rival->Spell = IJPSpellTests::MakeSpell(1, 0.5f, 0.5f, false);
	Mode->SetRival(Rival);
	UIJPAbilityComponent* RivalAbilities = Arena->GetPaddle(EIJPSide::Right)->GetAbilities();
	IJPSpellTests::Charge(Arena, RivalAbilities, 1);
	Arena->GetBall()->Serve(EIJPSide::Left, 0.f);
	Test.Step();
	UTEST_EQUAL("It cast", RivalAbilities->GetCharge(EIJPAbilitySlot::Spell), 0);
	bool bAtPlayer = false;
	for (const TWeakObjectPtr<AIJPSpellStrike>& Strike : Arena->GetStrikes())
	{
		bAtPlayer |= Strike.IsValid() && Strike->GetTargetSide() == EIJPSide::Left;
	}
	UTEST_TRUE("At the player", bAtPlayer);
	return true;
}

#endif
