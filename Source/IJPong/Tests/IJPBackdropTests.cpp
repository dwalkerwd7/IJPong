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
#include "Presentation/IJPBackdrop.h"
#include "Presentation/IJPBackdropComponent.h"
#include "Tests/IJPTestWorld.h"

namespace IJPBackdropTests
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
		Rules->ServeDelay = 30.f; // No serve during the test: only damage we apply.
		return Rules;
	}

	/** Two layers: a still sky, and debris that falls and has a broken look. */
	UIJPBackdrop* MakeBackdrop()
	{
		UIJPBackdrop* Backdrop = NewObject<UIJPBackdrop>(GetTransientPackage());
		FIJPBackdropLayer& Sky = Backdrop->Layers.AddDefaulted_GetRef();
		Sky.Texture = UTexture2D::CreateTransient(4, 4);
		FIJPBackdropLayer& Debris = Backdrop->Layers.AddDefaulted_GetRef();
		Debris.Texture = UTexture2D::CreateTransient(4, 4);
		Debris.BrokenTexture = UTexture2D::CreateTransient(4, 4);
		Debris.Drift = FVector2D(0.f, -0.1f);
		Debris.BreakBurst = 5.f;
		return Backdrop;
	}

	UIJPEra* MakeSpriteEra(UIJPBackdrop* Backdrop)
	{
		UIJPEra* Era = NewObject<UIJPEra>(GetTransientPackage());
		Era->bShowSprites = true;
		if (Backdrop)
		{
			Era->Backdrops.Add(Backdrop);
		}
		return Era;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPBackdropPickTest, "IJPong.Backdrop.EachFightPicksItsScenery", IJPBackdropTests::Flags)
bool FIJPBackdropPickTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test;
	AIJPTestGameMode* Mode = IJPBackdropTests::GetMode(Test);
	AIJPArena* Arena = Mode->GetArena();
	UIJPBackdropComponent* Backdrop = Arena->GetBackdrop();
	UIJPBackdrop* EraScenery = IJPBackdropTests::MakeBackdrop();
	UIJPEraSubsystem* Eras = UIJPEraSubsystem::Get(Arena);

	Mode->RestartMatch(IJPBackdropTests::MakeRules(5.f));
	UTEST_FALSE("1972: a plain screen", Backdrop->IsShown());

	Eras->SetEra(IJPBackdropTests::MakeSpriteEra(EraScenery));
	Mode->RestartMatch(IJPBackdropTests::MakeRules(5.f));
	UTEST_TRUE("Sprite era: scenery", Backdrop->IsShown());
	UTEST_TRUE("The era's", Backdrop->GetBackdrop() == EraScenery);

	// A boss brings its own arena.
	UIJPRival* Boss = NewObject<UIJPRival>(GetTransientPackage());
	Boss->Backdrop = IJPBackdropTests::MakeBackdrop();
	Mode->SetRival(Boss);
	Mode->RestartMatch(IJPBackdropTests::MakeRules(5.f));
	UTEST_TRUE("The boss's own", Backdrop->GetBackdrop() == Boss->Backdrop);

	// An era without scenery keeps the screen plain.
	Mode->SetRival(nullptr);
	Eras->SetEra(IJPBackdropTests::MakeSpriteEra(nullptr));
	Mode->RestartMatch(IJPBackdropTests::MakeRules(5.f));
	UTEST_FALSE("None to pick", Backdrop->IsShown());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPBackdropLifeTest, "IJPong.Backdrop.DriftsAndShakesOnHits", IJPBackdropTests::Flags)
bool FIJPBackdropLifeTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test;
	AIJPTestGameMode* Mode = IJPBackdropTests::GetMode(Test);
	AIJPArena* Arena = Mode->GetArena();
	UIJPBackdropComponent* Backdrop = Arena->GetBackdrop();
	UIJPEraSubsystem::Get(Arena)->SetEra(IJPBackdropTests::MakeSpriteEra(IJPBackdropTests::MakeBackdrop()));
	Mode->RestartMatch(IJPBackdropTests::MakeRules(5.f));
	UIJPMatchComponent* Match = Mode->GetMatch();

	Test.RunFor(1.f);
	UTEST_EQUAL_TOLERANCE("The debris fell a tenth of the screen", static_cast<float>(Backdrop->GetScroll(1).Y), -0.1f, 0.01f);
	UTEST_EQUAL("The sky stays put", Backdrop->GetScroll(0), FVector2D::ZeroVector);

	UTEST_FALSE("Calm", Backdrop->IsShaking());
	Match->ApplyDamage(EIJPSide::Left, 1.f);
	UTEST_TRUE("A hit shakes it", Backdrop->IsShaking());
	Test.RunFor(Backdrop->ShakeTime + 0.1f);
	UTEST_FALSE("Settles", Backdrop->IsShaking());
	Match->Heal(EIJPSide::Left, 1.f);
	UTEST_FALSE("A heal doesn't", Backdrop->IsShaking());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPBackdropBreakTest, "IJPong.Backdrop.TheBossArenaCrumblesWithIt", IJPBackdropTests::Flags)
bool FIJPBackdropBreakTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test;
	AIJPTestGameMode* Mode = IJPBackdropTests::GetMode(Test);
	AIJPArena* Arena = Mode->GetArena();
	UIJPBackdropComponent* Backdrop = Arena->GetBackdrop();
	UIJPEraSubsystem::Get(Arena)->SetEra(IJPBackdropTests::MakeSpriteEra(nullptr));

	UIJPRival* Boss = NewObject<UIJPRival>(GetTransientPackage());
	Boss->Backdrop = IJPBackdropTests::MakeBackdrop();
	FIJPBossPhase& Crumble = Boss->Phases.AddDefaulted_GetRef();
	Crumble.AtHealth = 0.5f;
	Crumble.SplitGap = 40.f;
	Mode->SetRival(Boss);
	Mode->RestartMatch(IJPBackdropTests::MakeRules(4.f));
	UTEST_FALSE("Whole", Backdrop->IsBroken());
	UTEST_TRUE("Its first look", Backdrop->GetShownTexture(1) == Boss->Backdrop->Layers[1].Texture);

	Mode->GetMatch()->ApplyDamage(EIJPSide::Right, 2.f);
	UTEST_TRUE("Crumbles as the boss breaks in two", Backdrop->IsBroken());
	UTEST_TRUE("The broken look", Backdrop->GetShownTexture(1) == Boss->Backdrop->Layers[1].BrokenTexture);
	UTEST_TRUE("The sky has no broken look, so it stays", Backdrop->GetShownTexture(0) == Boss->Backdrop->Layers[0].Texture);
	UTEST_TRUE("With a shake", Backdrop->IsShaking());
	Test.RunFor(0.5f);
	UTEST_TRUE("And a burst of debris", Backdrop->GetScroll(1).Y < -0.1f * 0.5f * 2.f);

	Mode->RestartMatch(IJPBackdropTests::MakeRules(4.f));
	UTEST_FALSE("A new fight starts whole", Backdrop->IsBroken());
	return true;
}

#endif
