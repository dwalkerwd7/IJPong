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
#include "Gameplay/IJPHealthBarComponent.h"
#include "Gameplay/IJPMatchComponent.h"
#include "Gameplay/IJPMatchRules.h"
#include "Gameplay/IJPSevenSegmentComponent.h"
#include "Tests/IJPTestWorld.h"

namespace IJPHealthBarTests
{
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter;

	AIJPTestGameMode* GetMode(const FIJPTestWorld& Test)
	{
		return Cast<AIJPTestGameMode>(Test.GetWorld()->GetAuthGameMode());
	}

	UIJPMatchRules* MakeRules(float Health)
	{
		UIJPMatchRules* Rules = NewObject<UIJPMatchRules>();
		Rules->StartingHealth = Health;
		Rules->ServeDelay = 5.f; // No serve during the test: only damage we apply.
		return Rules;
	}

	/** A sprite era with a bar: a 160 x 20 unit frame and a smooth fill inside it (or a segmented one). */
	UIJPEra* MakeBarEra(int32 Segments = 0, float SegmentPitch = 0.f)
	{
		UIJPEra* Era = NewObject<UIJPEra>(GetTransientPackage());
		Era->bShowSprites = true;
		Era->HealthBar.Frame = UTexture2D::CreateTransient(1280, 160);
		Era->HealthBar.Fill = UTexture2D::CreateTransient(1232, 112);
		Era->HealthBar.FillOffset = FVector2D(24.f, 24.f);
		Era->HealthBar.Segments = Segments;
		Era->HealthBar.SegmentPitch = SegmentPitch;
		return Era;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPHealthBarShownTest, "IJPong.HealthBar.BarsReplaceTheNumbersInBarEras", IJPHealthBarTests::Flags)
bool FIJPHealthBarShownTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test;
	AIJPTestGameMode* Mode = IJPHealthBarTests::GetMode(Test);
	AIJPArena* Arena = Mode->GetArena();
	Mode->RestartMatch(IJPHealthBarTests::MakeRules(5.f));
	UTEST_FALSE("1972: numbers", Arena->GetHealthBar(EIJPSide::Left)->IsShown());
	UTEST_TRUE("1972: the number is up", Arena->GetScoreDisplay(EIJPSide::Left)->IsVisible());

	UIJPEraSubsystem::Get(Arena)->SetEra(IJPHealthBarTests::MakeBarEra());
	for (const EIJPSide Side : { EIJPSide::Left, EIJPSide::Right })
	{
		UTEST_TRUE("A bar", Arena->GetHealthBar(Side)->IsShown());
		UTEST_FALSE("No number", Arena->GetScoreDisplay(Side)->IsVisible());
	}

	// Each hugs its own side's outer edge, and the right one is mirrored so both drain toward the net.
	const UIJPHealthBarComponent* Left = Arena->GetHealthBar(EIJPSide::Left);
	const UIJPHealthBarComponent* Right = Arena->GetHealthBar(EIJPSide::Right);
	const float HalfWidth = Arena->GetHalfExtents().X;
	UTEST_TRUE("Left bar at the left edge", Left->GetRelativeLocation().X < -HalfWidth * 0.9f);
	UTEST_TRUE("Right bar at the right edge", Right->GetRelativeLocation().X > HalfWidth * 0.9f);
	UTEST_TRUE("Right bar mirrored", Right->GetRelativeScale3D().X < 0.f && Left->GetRelativeScale3D().X > 0.f);
	UTEST_EQUAL("Sized from the art at 8 px a unit", Left->GetSize(), FVector2D(160.f, 20.f));

	// Each bar has its own pieces (they share the arena as owner).
	TArray<USceneComponent*> LeftPieces, RightPieces;
	Left->GetChildrenComponents(false, LeftPieces);
	Right->GetChildrenComponents(false, RightPieces);
	UTEST_EQUAL("Left: frame, trail, fill", LeftPieces.Num(), 3);
	UTEST_EQUAL("Right: frame, trail, fill", RightPieces.Num(), 3);
	UTEST_TRUE("Left pieces drawn", LeftPieces.Num() > 0 && LeftPieces[0]->IsVisible());

	// An endless match has no health: back to numbers (they count goals).
	UIJPMatchRules* Endless = IJPHealthBarTests::MakeRules(0.f);
	Mode->RestartMatch(Endless);
	UTEST_FALSE("Endless: numbers", Left->IsShown());
	UTEST_TRUE("Endless: the number is up", Arena->GetScoreDisplay(EIJPSide::Left)->IsVisible());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPHealthBarTrailTest, "IJPong.HealthBar.ALostChunkLingersThenDrains", IJPHealthBarTests::Flags)
bool FIJPHealthBarTrailTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test;
	AIJPTestGameMode* Mode = IJPHealthBarTests::GetMode(Test);
	AIJPArena* Arena = Mode->GetArena();
	UIJPEraSubsystem::Get(Arena)->SetEra(IJPHealthBarTests::MakeBarEra());
	Mode->RestartMatch(IJPHealthBarTests::MakeRules(5.f));
	UIJPMatchComponent* Match = Mode->GetMatch();
	UIJPHealthBarComponent* Bar = Arena->GetHealthBar(EIJPSide::Left);
	UTEST_EQUAL("Full", Bar->GetFraction(), 1.f);

	Match->ApplyDamage(EIJPSide::Left, 1.f);
	UTEST_EQUAL("The fill drops at once", Bar->GetFraction(), 0.8f);
	UTEST_EQUAL("The trail holds what was lost", Bar->GetTrailFraction(), 1.f);
	UTEST_FALSE("No blinking number on a hit", Arena->GetScoreDisplay(EIJPSide::Left)->IsFlashing());
	Test.RunFor(Bar->TrailHold * 0.5f);
	UTEST_EQUAL("Still holding", Bar->GetTrailFraction(), 1.f);
	Test.RunFor(Bar->TrailHold + 1.f / Bar->TrailSpeed);
	UTEST_EQUAL("Then drains to the fill", Bar->GetTrailFraction(), 0.8f);

	Match->Heal(EIJPSide::Left, 0.5f);
	UTEST_EQUAL("A heal fills back up", Bar->GetFraction(), 0.9f);
	UTEST_EQUAL("And takes the trail along", Bar->GetTrailFraction(), 0.9f);

	// A set health (the run's, at a fight's start) isn't a hit: no trail.
	Match->SetHealth(EIJPSide::Left, 2.f, 5.f);
	UTEST_EQUAL("Set", Bar->GetFraction(), 0.4f);
	UTEST_EQUAL("No trail for a set", Bar->GetTrailFraction(), 0.4f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPHealthBarSegmentTest, "IJPong.HealthBar.SegmentedFillsCutBetweenSegments", IJPHealthBarTests::Flags)
bool FIJPHealthBarSegmentTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test;
	AIJPTestGameMode* Mode = IJPHealthBarTests::GetMode(Test);
	AIJPArena* Arena = Mode->GetArena();
	UIJPEraSubsystem::Get(Arena)->SetEra(IJPHealthBarTests::MakeBarEra(25, 48.f));
	Mode->RestartMatch(IJPHealthBarTests::MakeRules(5.f));
	UIJPHealthBarComponent* Bar = Arena->GetHealthBar(EIJPSide::Right);

	// 4.5 of 5 = 22.5 of 25 segments: the part-used one stays lit, and the cut lands after it.
	Mode->GetMatch()->ApplyDamage(EIJPSide::Right, 0.5f);
	UTEST_EQUAL("23 segments lit", Bar->GetFillCut(), 23.f * 48.f / 1232.f);
	Mode->GetMatch()->ApplyDamage(EIJPSide::Right, 1.5f);
	UTEST_EQUAL("3 of 5 = 15 segments", Bar->GetFillCut(), 15.f * 48.f / 1232.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPHealthBarWinnerTest, "IJPong.HealthBar.TheWinnersBarBlinks", IJPHealthBarTests::Flags)
bool FIJPHealthBarWinnerTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test;
	AIJPTestGameMode* Mode = IJPHealthBarTests::GetMode(Test);
	AIJPArena* Arena = Mode->GetArena();
	UIJPEraSubsystem::Get(Arena)->SetEra(IJPHealthBarTests::MakeBarEra());
	Mode->RestartMatch(IJPHealthBarTests::MakeRules(1.f));

	Mode->GetMatch()->ApplyDamage(EIJPSide::Right, 1.f);
	UTEST_TRUE("Over", Mode->GetMatch()->IsOver());
	UTEST_TRUE("The winner's bar blinks", Arena->GetHealthBar(EIJPSide::Left)->IsFlashing());
	UTEST_FALSE("The loser's doesn't", Arena->GetHealthBar(EIJPSide::Right)->IsFlashing());
	UTEST_FALSE("No number shows up", Arena->GetScoreDisplay(EIJPSide::Left)->IsVisible());

	Mode->RestartMatch(IJPHealthBarTests::MakeRules(1.f));
	UTEST_FALSE("A new match stops it", Arena->GetHealthBar(EIJPSide::Left)->IsFlashing());
	UTEST_EQUAL("And both are full", Arena->GetHealthBar(EIJPSide::Right)->GetFraction(), 1.f);
	return true;
}

#endif
