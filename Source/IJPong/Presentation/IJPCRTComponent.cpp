// It's Just Pong

#include "Presentation/IJPCRTComponent.h"
#include "Camera/CameraComponent.h"
#include "Core/IJPTypes.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace
{
	const FName FlashParam(TEXT("Flash"));
}

UIJPCRTComponent::UIJPCRTComponent()
{
	// Ticks only while a pulse is fading.
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UIJPCRTComponent::Pulse(float Strength, float Duration)
{
	PulseStrength = Strength;
	PulseDuration = FMath::Max(Duration, UE_KINDA_SMALL_NUMBER);
	PulseElapsed = 0.f;
	SetFlash(PulseStrength);
	SetComponentTickEnabled(true);
}

void UIJPCRTComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	PulseElapsed += DeltaTime;
	const float Remaining = 1.f - FMath::Clamp(PulseElapsed / PulseDuration, 0.f, 1.f);
	// Squared falloff: a sharp flash that dies away quickly, like a tube's afterglow.
	SetFlash(PulseStrength * Remaining * Remaining);
	if (Remaining <= 0.f)
	{
		SetComponentTickEnabled(false);
	}
}

void UIJPCRTComponent::SetFlash(float Value)
{
	CurrentFlash = Value;
	if (MaterialInstance)
	{
		MaterialInstance->SetScalarParameterValue(FlashParam, Value);
	}
}

void UIJPCRTComponent::BeginPlay()
{
	Super::BeginPlay();

	UMaterialInterface* Base = CRTMaterial.LoadSynchronous();
	if (!Base)
	{
		UE_LOG(LogIJPong, Warning, TEXT("%s: no CRT material set; no CRT look."), *GetPathName());
		return;
	}

	// Applied only at runtime: editing the cameras' settings in the editor would save a transient
	// material instance into the level.
	MaterialInstance = UMaterialInstanceDynamic::Create(Base, this);
	TArray<UCameraComponent*> Cameras;
	GetCameras(Cameras);
	for (UCameraComponent* Camera : Cameras)
	{
		Camera->PostProcessSettings.WeightedBlendables.Array.Add(FWeightedBlendable(bEnabled ? 1.f : 0.f, MaterialInstance));
	}
}

void UIJPCRTComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (MaterialInstance)
	{
		TArray<UCameraComponent*> Cameras;
		GetCameras(Cameras);
		for (UCameraComponent* Camera : Cameras)
		{
			Camera->PostProcessSettings.WeightedBlendables.Array.RemoveAll([this](const FWeightedBlendable& Blendable)
			{
				return Blendable.Object == MaterialInstance;
			});
		}
		MaterialInstance = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}

void UIJPCRTComponent::SetCRTEnabled(bool bEnable)
{
	bEnabled = bEnable;
	ApplyWeight(bEnabled ? 1.f : 0.f);
}

void UIJPCRTComponent::ApplyWeight(float Weight)
{
	if (!MaterialInstance)
	{
		return;
	}

	TArray<UCameraComponent*> Cameras;
	GetCameras(Cameras);
	for (UCameraComponent* Camera : Cameras)
	{
		for (FWeightedBlendable& Blendable : Camera->PostProcessSettings.WeightedBlendables.Array)
		{
			if (Blendable.Object == MaterialInstance)
			{
				Blendable.Weight = Weight;
			}
		}
	}
}

void UIJPCRTComponent::GetCameras(TArray<UCameraComponent*>& OutCameras) const
{
	if (const AActor* Owner = GetOwner())
	{
		Owner->GetComponents(OutCameras);
	}
}
