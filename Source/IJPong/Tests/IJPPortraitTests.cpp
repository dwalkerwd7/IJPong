// It's Just Pong

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Core/IJPTestGameMode.h"
#include "Core/IJPTypes.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "Era/IJPEra.h"
#include "Era/IJPEraSubsystem.h"
#include "Gameplay/IJPArena.h"
#include "Gameplay/IJPMatchComponent.h"
#include "Gameplay/IJPMatchRules.h"
#include "Gameplay/IJPRival.h"
#include "Narrative/IJPConversation.h"
#include "Narrative/IJPConversationPlayer.h"
#include "Narrative/IJPPortraitComponent.h"
#include "Tests/IJPTestWorld.h"

namespace IJPPortraitTests
{
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter;

	AIJPTestGameMode* GetMode(const FIJPTestWorld& Test)
	{
		return Cast<AIJPTestGameMode>(Test.GetWorld()->GetAuthGameMode());
	}

	UIJPRival* MakeRival()
	{
		UIJPRival* Rival = NewObject<UIJPRival>(GetTransientPackage());
		Rival->Portraits.Neutral = UTexture2D::CreateTransient(8, 8);
		Rival->Portraits.Smug = UTexture2D::CreateTransient(8, 8);
		Rival->Portraits.Rattled = UTexture2D::CreateTransient(8, 8);
		return Rival;
	}

	UIJPMatchRules* MakeRules(float Health)
	{
		UIJPMatchRules* Rules = NewObject<UIJPMatchRules>();
		Rules->StartingHealth = Health;
		Rules->ServeDelay = 30.f; // No serve during the test: only damage we apply.
		return Rules;
	}

	void UseSpriteEra(AIJPArena* Arena)
	{
		UIJPEra* Era = NewObject<UIJPEra>(GetTransientPackage());
		Era->bShowSprites = true;
		UIJPEraSubsystem::Get(Arena)->SetEra(Era);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPPortraitShownTest, "IJPong.Portraits.TheRivalsFaceShowsInSpriteEras", IJPPortraitTests::Flags)
bool FIJPPortraitShownTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test;
	AIJPTestGameMode* Mode = IJPPortraitTests::GetMode(Test);
	AIJPArena* Arena = Mode->GetArena();
	Mode->SetRival(IJPPortraitTests::MakeRival());
	Mode->RestartMatch(IJPPortraitTests::MakeRules(5.f));
	UIJPPortraitComponent* Rival = Arena->GetPortrait(EIJPSide::Right);
	UTEST_FALSE("1972: no faces", Rival->IsFaceShown());

	IJPPortraitTests::UseSpriteEra(Arena);
	UTEST_TRUE("Sprite era: the rival's face", Rival->IsFaceShown());
	UTEST_FALSE("The player has no portrait", Arena->GetPortrait(EIJPSide::Left)->IsFaceShown());
	UTEST_EQUAL("Level: neutral", Rival->GetExpression(), EIJPExpression::Neutral);
	UTEST_TRUE("Up in the rival's top corner", Rival->GetRelativeLocation().X > 0.f && Rival->GetRelativeLocation().Z > Arena->GetHalfExtents().Y * 0.5f);

	Mode->SetRival(nullptr);
	UTEST_FALSE("No rival, no face", Rival->IsFaceShown());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPPortraitMoodTest, "IJPong.Portraits.TheFaceFollowsTheHealthRace", IJPPortraitTests::Flags)
bool FIJPPortraitMoodTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test;
	AIJPTestGameMode* Mode = IJPPortraitTests::GetMode(Test);
	AIJPArena* Arena = Mode->GetArena();
	IJPPortraitTests::UseSpriteEra(Arena);
	Mode->SetRival(IJPPortraitTests::MakeRival());
	Mode->RestartMatch(IJPPortraitTests::MakeRules(5.f));
	UIJPMatchComponent* Match = Mode->GetMatch();
	UIJPPortraitComponent* Rival = Arena->GetPortrait(EIJPSide::Right);

	Match->ApplyDamage(EIJPSide::Left, 1.f);
	UTEST_EQUAL("Ahead: smug", Rival->GetExpression(), EIJPExpression::Smug);
	Match->ApplyDamage(EIJPSide::Right, 2.f);
	UTEST_EQUAL("Behind: rattled", Rival->GetExpression(), EIJPExpression::Rattled);
	Match->Heal(EIJPSide::Right, 1.f);
	UTEST_EQUAL("Level again: neutral", Rival->GetExpression(), EIJPExpression::Neutral);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPPortraitLineTest, "IJPong.Portraits.ALineCanPickTheFace", IJPPortraitTests::Flags)
bool FIJPPortraitLineTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test;
	AIJPTestGameMode* Mode = IJPPortraitTests::GetMode(Test);
	AIJPArena* Arena = Mode->GetArena();
	IJPPortraitTests::UseSpriteEra(Arena);
	Mode->SetRival(IJPPortraitTests::MakeRival());
	Mode->RestartMatch(IJPPortraitTests::MakeRules(5.f));
	Mode->GetMatch()->ApplyDamage(EIJPSide::Right, 1.f);
	UIJPPortraitComponent* Rival = Arena->GetPortrait(EIJPSide::Right);
	UTEST_EQUAL("Behind: rattled", Rival->GetExpression(), EIJPExpression::Rattled);

	// Putting on a brave face: the line says smug, whatever the score.
	UIJPConversation* Brave = NewObject<UIJPConversation>(GetTransientPackage());
	FIJPConversationLine& Line = Brave->Lines.AddDefaulted_GetRef();
	Line.Speaker = EIJPSpeaker::Opponent;
	Line.Text = FText::FromString(TEXT("LUCKY"));
	Line.HoldTime = 0.3f;
	Line.Expression = EIJPExpression::Smug;
	Mode->PlayConversation(Brave);
	UTEST_EQUAL("The line's face", Rival->GetExpression(), EIJPExpression::Smug);

	for (int32 i = 0; i < 600 && Mode->GetConversations()->IsPlaying(); ++i)
	{
		Test.Step();
	}
	UTEST_FALSE("Said", Mode->GetConversations()->IsPlaying());
	UTEST_EQUAL("Then back to the mood", Rival->GetExpression(), EIJPExpression::Rattled);
	return true;
}

#endif
