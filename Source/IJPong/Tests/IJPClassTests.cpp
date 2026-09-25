// It's Just Pong

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Abilities/IJPAbility_Grow.h"
#include "Abilities/IJPAbilityComponent.h"
#include "AI/IJPAIProfile.h"
#include "AI/IJPPaddleAIController.h"
#include "Core/IJPTestGameMode.h"
#include "Core/IJPTypes.h"
#include "Engine/World.h"
#include "Gameplay/IJPArena.h"
#include "Gameplay/IJPPaddle.h"
#include "Gameplay/IJPPaddleClass.h"
#include "Gameplay/IJPPaddleProfile.h"
#include "Gameplay/IJPRival.h"
#include "Tests/IJPTestWorld.h"

namespace IJPClassTests
{
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter;

	UIJPPaddleClass* MakeClass(float Length, UIJPAbility* Skill = nullptr)
	{
		UIJPPaddleProfile* Profile = NewObject<UIJPPaddleProfile>(GetTransientPackage());
		Profile->Size.Y = Length;
		UIJPPaddleClass* PaddleClass = NewObject<UIJPPaddleClass>(GetTransientPackage());
		PaddleClass->Profile = Profile;
		PaddleClass->ClassSkill = Skill;
		return PaddleClass;
	}

	AIJPTestGameMode* GetMode(const FIJPTestWorld& Test)
	{
		return Cast<AIJPTestGameMode>(Test.GetWorld()->GetAuthGameMode());
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPClassSwitchTest, "IJPong.Class.SwitchingChangesStatsAndSkillLive", IJPClassTests::Flags)
bool FIJPClassSwitchTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test;
	AIJPArena* Arena = Test.GetArena();
	AIJPPaddle* Left = Arena->GetPaddle(EIJPSide::Left);

	// Resting against the top wall, then switching to a much longer class.
	Test.RunFor(0.6f, [Left] { Left->AddMoveInput(1.f); });
	UIJPPaddleClass* Tall = IJPClassTests::MakeClass(160.f, NewObject<UIJPAbility_Grow>(GetTransientPackage()));
	Left->SetPaddleClass(Tall);

	UTEST_EQUAL("Class set", Left->GetPaddleClass(), static_cast<const UIJPPaddleClass*>(Tall));
	UTEST_EQUAL_TOLERANCE("The class's length", Left->GetSize().Y, 160.0, 0.01);
	UTEST_TRUE("Pushed back inside the walls", Left->GetPlanePosition().Y + Left->GetSize().Y * 0.5 <= Arena->GetHalfExtents().Y + 0.01);
	const UIJPAbility* Skill = Left->GetAbilities()->GetAbility(EIJPAbilitySlot::ClassSkill);
	UTEST_TRUE("The class's skill", Skill && Skill->IsA<UIJPAbility_Grow>());

	Left->SetPaddleClass(nullptr);
	UTEST_NULL("No class: no skill", Left->GetAbilities()->GetAbility(EIJPAbilitySlot::ClassSkill));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPRivalTest, "IJPong.Rival.SetsTheOpponentsClassAndStyle", IJPClassTests::Flags)
bool FIJPRivalTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test;
	AIJPTestGameMode* Mode = IJPClassTests::GetMode(Test);
	AIJPArena* Arena = Mode->GetArena();
	AIJPPaddle* Right = Arena->GetPaddle(EIJPSide::Right);
	AIJPPaddleAIController* AI = Cast<AIJPPaddleAIController>(Right->GetController());
	UTEST_NOT_NULL("Opponent AI", AI);
	const UIJPPaddleClass* ArenaClass = Right->GetPaddleClass();
	const UIJPAIProfile* DefaultStyle = &AI->GetProfile();

	UIJPRival* Rival = NewObject<UIJPRival>(GetTransientPackage());
	Rival->PaddleClass = IJPClassTests::MakeClass(100.f);
	Rival->AIProfile = NewObject<UIJPAIProfile>(GetTransientPackage());
	const float SkillBefore = AI->GetSkill();

	Mode->SetRival(Rival);
	UTEST_EQUAL("Rival's class on the opponent paddle", Right->GetPaddleClass(), static_cast<const UIJPPaddleClass*>(Rival->PaddleClass.Get()));
	UTEST_EQUAL("Rival's style on its AI", &AI->GetProfile(), static_cast<const UIJPAIProfile*>(Rival->AIProfile.Get()));
	UTEST_EQUAL("Difficulty stays the arena's", AI->GetSkill(), SkillBefore);
	UTEST_NOT_EQUAL("Player's paddle untouched", Arena->GetPaddle(EIJPSide::Left)->GetPaddleClass(), static_cast<const UIJPPaddleClass*>(Rival->PaddleClass.Get()));

	Mode->SetRival(nullptr);
	UTEST_EQUAL("No rival: the arena's class again", Right->GetPaddleClass(), ArenaClass);
	UTEST_EQUAL("And the default style", &AI->GetProfile(), DefaultStyle);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPClassKeysTest, "IJPong.TestMode.ClassAndRivalKeysCycle", IJPClassTests::Flags)
bool FIJPClassKeysTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test;
	AIJPTestGameMode* Mode = IJPClassTests::GetMode(Test);
	AIJPPaddle* Left = Mode->GetArena()->GetPaddle(EIJPSide::Left);

	const UIJPPaddleClass* Before = Left->GetPaddleClass();
	Mode->CyclePlayerClass(1);
	UTEST_NOT_EQUAL("Class key changes your class", Left->GetPaddleClass(), Before);

	UTEST_NULL("No rival to start with", Mode->GetRival());
	Mode->CycleRival(1);
	UTEST_NOT_NULL("Rival key picks the first rival", Mode->GetRival());
	Mode->CycleRival(-1);
	UTEST_NULL("And back to no rival", Mode->GetRival());
	return true;
}

#endif
