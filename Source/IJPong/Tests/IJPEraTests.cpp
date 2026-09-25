// It's Just Pong

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Audio/IJPToneSet.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/IJPTestGameMode.h"
#include "Engine/World.h"
#include "Era/IJPEra.h"
#include "Era/IJPEraSubsystem.h"
#include "Gameplay/IJPArena.h"
#include "Gameplay/IJPBall.h"
#include "Gameplay/IJPPaddle.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Presentation/IJPCRTComponent.h"
#include "Tests/IJPTestWorld.h"

namespace IJPEraTests
{
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter;

	/** The "Color" the material on an actor's mesh is painted with. */
	FLinearColor ColorOf(const AActor* Actor)
	{
		FLinearColor Color = FLinearColor::Transparent;
		const UStaticMeshComponent* Mesh = Actor ? Actor->FindComponentByClass<UStaticMeshComponent>() : nullptr;
		if (const UMaterialInterface* Material = Mesh ? Mesh->GetMaterial(0) : nullptr)
		{
			Material->GetVectorParameterValue(FHashedMaterialParameterInfo(TEXT("Color")), Color);
		}
		return Color;
	}

	FLinearColor ColorOf(const UMaterialInterface* Material)
	{
		FLinearColor Color = FLinearColor::Transparent;
		if (Material)
		{
			Material->GetVectorParameterValue(FHashedMaterialParameterInfo(TEXT("Color")), Color);
		}
		return Color;
	}

	/** The material the CRT component's live instance was made from; null when it shows no CRT. */
	const UMaterialInterface* CRTBase(const UIJPCRTComponent* CRT)
	{
		const UMaterialInstanceDynamic* Instance = CRT->GetCRTMaterial();
		return Instance ? Instance->Parent.Get() : nullptr;
	}

	bool HasBlendable(const UCameraComponent* Camera, const UObject* Material)
	{
		return Material && Camera->PostProcessSettings.WeightedBlendables.Array.ContainsByPredicate(
			[Material](const FWeightedBlendable& Blendable) { return Blendable.Object == Material; });
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPEraStartTest, "IJPong.Era.GameStartsInTheFirstEraAndShowsIt", IJPEraTests::Flags)
bool FIJPEraStartTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test;
	AIJPArena* Arena = Test.GetArena();
	UIJPEraSubsystem* Eras = UIJPEraSubsystem::Get(Arena);
	UTEST_NOT_NULL("Era subsystem on the game instance", Eras);
	UTEST_TRUE("Eras configured", Eras->GetNumEras() >= 2);
	UTEST_EQUAL("Starts in the first era", Eras->GetEraIndex(), 0);

	const UIJPEra* Era = Eras->GetEra();
	UTEST_NOT_NULL("First era's CRT", Era->CRTMaterial.Get());
	UTEST_EQUAL("Arena CRT uses the era's CRT", IJPEraTests::CRTBase(Arena->GetCRT()), static_cast<const UMaterialInterface*>(Era->CRTMaterial.Get()));
	UTEST_EQUAL("Arena beeps with the era's tones", &Arena->GetToneSet(), static_cast<const UIJPToneSet*>(Era->ToneSet.Get()));
	UTEST_EQUAL("Left paddle in the era's colour", IJPEraTests::ColorOf(Arena->GetPaddle(EIJPSide::Left)), Era->Palette.LeftPaddle);
	UTEST_EQUAL("Ball in the era's colour", IJPEraTests::ColorOf(Arena->GetBall()), Era->Palette.Ball);
	UTEST_EQUAL("Background in the era's colour", IJPEraTests::ColorOf(Arena->GetPaletteMaterial(EIJPPaletteRole::Background)), Era->Palette.Background);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPEraSwitchTest, "IJPong.Era.SwitchingUpgradesEverythingLive", IJPEraTests::Flags)
bool FIJPEraSwitchTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test;
	AIJPArena* Arena = Test.GetArena();
	UIJPEraSubsystem* Eras = UIJPEraSubsystem::Get(Arena);

	// A menu camera showing the era too, e.g. a front-end backdrop.
	ACameraActor* MenuCamera = Test.GetWorld()->SpawnActor<ACameraActor>();
	UIJPCRTComponent* MenuCRT = NewObject<UIJPCRTComponent>(MenuCamera);
	MenuCRT->RegisterComponent();
	UTEST_NOT_NULL("Menu camera starts with the era's CRT", IJPEraTests::CRTBase(MenuCRT));

	// A later era: its own colours and tones, and no CRT at all (like the Grid era).
	UIJPEra* Later = NewObject<UIJPEra>(GetTransientPackage());
	Later->Palette.Background = FLinearColor(0.f, 0.f, 0.2f);
	Later->Palette.LeftPaddle = FLinearColor::Green;
	Later->Palette.RightPaddle = FLinearColor::Red;
	Later->Palette.Ball = FLinearColor::Yellow;
	Later->ToneSet = NewObject<UIJPToneSet>(Later);
	Later->ToneSet->PaddleHit.Frequency = 999.f;

	Test.Step();
	Arena->GetBall()->Serve(EIJPSide::Left, 0.f);
	Test.RunFor(0.2f);
	Eras->SetEra(Later);
	Test.Step();

	UTEST_EQUAL("Left paddle recoloured", IJPEraTests::ColorOf(Arena->GetPaddle(EIJPSide::Left)), FLinearColor::Green);
	UTEST_EQUAL("Right paddle recoloured", IJPEraTests::ColorOf(Arena->GetPaddle(EIJPSide::Right)), FLinearColor::Red);
	UTEST_EQUAL("Ball recoloured mid-rally", IJPEraTests::ColorOf(Arena->GetBall()), FLinearColor::Yellow);
	UTEST_EQUAL("Background recoloured", IJPEraTests::ColorOf(Arena->GetPaletteMaterial(EIJPPaletteRole::Background)), FLinearColor(0.f, 0.f, 0.2f));
	UTEST_TRUE("Ball still in play", Arena->GetBall()->IsInPlay());
	UTEST_EQUAL("New tones", Arena->GetToneSet().PaddleHit.Frequency, 999.f);
	UTEST_NULL("Arena CRT gone", Arena->GetCRT()->GetCRTMaterial());
	UTEST_NULL("Menu CRT gone", MenuCRT->GetCRTMaterial());
	UTEST_EQUAL("Nothing left on the arena camera", Arena->GetCamera()->PostProcessSettings.WeightedBlendables.Array.Num(), 0);

	// And back to the first era: the CRT returns on both cameras.
	Eras->SetEraIndex(0);
	const UMaterialInterface* FirstCRT = Eras->GetEra()->CRTMaterial;
	UTEST_EQUAL("Arena CRT back", IJPEraTests::CRTBase(Arena->GetCRT()), FirstCRT);
	UTEST_EQUAL("Menu CRT back", IJPEraTests::CRTBase(MenuCRT), FirstCRT);
	UTEST_TRUE("On the arena camera", IJPEraTests::HasBlendable(Arena->GetCamera(), Arena->GetCRT()->GetCRTMaterial()));
	UTEST_TRUE("On the menu camera", IJPEraTests::HasBlendable(MenuCamera->GetCameraComponent(), MenuCRT->GetCRTMaterial()));
	UTEST_EQUAL("First era's paddle colour", IJPEraTests::ColorOf(Arena->GetPaddle(EIJPSide::Left)), Eras->GetEra()->Palette.LeftPaddle);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPEraCycleTest, "IJPong.TestMode.EraKeysCycleAndWrap", IJPEraTests::Flags)
bool FIJPEraCycleTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test;
	AIJPTestGameMode* Mode = Cast<AIJPTestGameMode>(Test.GetWorld()->GetAuthGameMode());
	UTEST_NOT_NULL("Test game mode", Mode);
	UIJPEraSubsystem* Eras = UIJPEraSubsystem::Get(Mode);
	const int32 Last = Eras->GetNumEras() - 1;

	Mode->CycleEra(-1);
	UTEST_EQUAL("Back from the first wraps to the last", Eras->GetEraIndex(), Last);
	Mode->CycleEra(1);
	UTEST_EQUAL("On from the last wraps to the first", Eras->GetEraIndex(), 0);
	Mode->CycleEra(1);
	UTEST_EQUAL("Next", Eras->GetEraIndex(), 1);
	return true;
}

#endif
