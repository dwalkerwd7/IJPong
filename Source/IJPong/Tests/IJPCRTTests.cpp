// It's Just Pong

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Engine/World.h"
#include "Gameplay/IJPArena.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Presentation/IJPCRTComponent.h"
#include "Tests/IJPTestWorld.h"

namespace IJPCRTTests
{
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter;

	/** Weight of Material in Camera's post-process blendables, or -1 if it isn't there. */
	float BlendWeight(const UCameraComponent* Camera, const UObject* Material)
	{
		for (const FWeightedBlendable& Blendable : Camera->PostProcessSettings.WeightedBlendables.Array)
		{
			if (Material && Blendable.Object == Material)
			{
				return Blendable.Weight;
			}
		}
		return -1.f;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPCRTArenaTest, "IJPong.CRT.ArenaCameraGetsTheLook", IJPCRTTests::Flags)
bool FIJPCRTArenaTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test;
	AIJPArena* Arena = Test.GetArena();
	UIJPCRTComponent* CRT = Arena->GetCRT();
	UTEST_NOT_NULL("Arena has a CRT component", CRT);
	UTEST_NOT_NULL("Configured CRT material loaded", CRT->GetCRTMaterial());

	UCameraComponent* Camera = Arena->GetCamera();
	UTEST_EQUAL("On the arena camera at full weight", IJPCRTTests::BlendWeight(Camera, CRT->GetCRTMaterial()), 1.f);

	CRT->SetCRTEnabled(false);
	UTEST_EQUAL("Disabled = weight 0", IJPCRTTests::BlendWeight(Camera, CRT->GetCRTMaterial()), 0.f);
	CRT->SetCRTEnabled(true);
	UTEST_EQUAL("Re-enabled = weight 1", IJPCRTTests::BlendWeight(Camera, CRT->GetCRTMaterial()), 1.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPCRTAnyCameraTest, "IJPong.CRT.WorksOnAnyCameraActor", IJPCRTTests::Flags)
bool FIJPCRTAnyCameraTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test;

	// e.g. a menu's backdrop camera: a plain CameraActor with the component added at runtime.
	ACameraActor* MenuCamera = Test.GetWorld()->SpawnActor<ACameraActor>();
	UIJPCRTComponent* CRT = NewObject<UIJPCRTComponent>(MenuCamera);
	CRT->RegisterComponent(); // the actor has begun play, so this also begins play on the component
	UTEST_NOT_NULL("Material instance created", CRT->GetCRTMaterial());

	UCameraComponent* Camera = MenuCamera->GetCameraComponent();
	UMaterialInstanceDynamic* Material = CRT->GetCRTMaterial();
	UTEST_EQUAL("Applied to the camera actor's camera", IJPCRTTests::BlendWeight(Camera, Material), 1.f);
	UTEST_TRUE("Independent of the arena's own instance", Material != Test.GetArena()->GetCRT()->GetCRTMaterial());

	CRT->DestroyComponent();
	UTEST_EQUAL("Removed again when the component goes", IJPCRTTests::BlendWeight(Camera, Material), -1.f);
	return true;
}

#endif
