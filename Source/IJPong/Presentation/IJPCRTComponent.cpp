// It's Just Pong

#include "Presentation/IJPCRTComponent.h"
#include "Camera/CameraComponent.h"
#include "Core/IJPTypes.h"
#include "Era/IJPEra.h"
#include "Era/IJPEraSubsystem.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace
{
	const FName FlashParam(TEXT("Flash"));
	const FName NoiseParam(TEXT("NoiseStrength"));
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

	bool bBusy = false;
	if (PulseElapsed < PulseDuration)
	{
		PulseElapsed += DeltaTime;
		const float Remaining = 1.f - FMath::Clamp(PulseElapsed / PulseDuration, 0.f, 1.f);
		// Squared falloff: a sharp flash that dies away quickly, like a tube's afterglow.
		SetFlash(PulseStrength * Remaining * Remaining);
		bBusy |= Remaining > 0.f;
	}
	if (JamElapsed < JamDuration)
	{
		JamElapsed += DeltaTime;
		const float Remaining = 1.f - FMath::Clamp(JamElapsed / JamDuration, 0.f, 1.f);
		// Full static for most of it, fading out over the last quarter.
		SetJam(JamStrength * FMath::Min(Remaining * 4.f, 1.f));
		bBusy |= Remaining > 0.f;
	}
	if (!bBusy)
	{
		SetComponentTickEnabled(false);
	}
}

void UIJPCRTComponent::Jam(float Strength, float Duration)
{
	JamStrength = Strength;
	JamDuration = FMath::Max(Duration, UE_KINDA_SMALL_NUMBER);
	JamElapsed = 0.f;
	SetJam(JamStrength);
	SetComponentTickEnabled(true);
}

void UIJPCRTComponent::SetJam(float Value)
{
	CurrentJam = Value;
	if (MaterialInstance)
	{
		MaterialInstance->SetScalarParameterValue(NoiseParam, BaseNoise + Value);
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

	if (!CRTMaterial.IsNull())
	{
		SetBaseMaterial(CRTMaterial.LoadSynchronous());
		return;
	}

	const UIJPEra* Era = UIJPEraSubsystem::GetCurrentEra(this);
	SetBaseMaterial(Era ? Era->CRTMaterial.Get() : nullptr);
	if (UIJPEraSubsystem* Eras = UIJPEraSubsystem::Get(this))
	{
		Eras->OnEraChanged.AddDynamic(this, &UIJPCRTComponent::HandleEraChanged);
	}
}

void UIJPCRTComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UIJPEraSubsystem* Eras = UIJPEraSubsystem::Get(this))
	{
		Eras->OnEraChanged.RemoveAll(this);
	}
	RemoveFromCameras();

	Super::EndPlay(EndPlayReason);
}

void UIJPCRTComponent::HandleEraChanged(const UIJPEra* NewEra)
{
	SetBaseMaterial(NewEra ? NewEra->CRTMaterial.Get() : nullptr);
}

void UIJPCRTComponent::SetBaseMaterial(UMaterialInterface* Base)
{
	RemoveFromCameras();
	if (!Base)
	{
		return;
	}

	// Applied only at runtime: editing the cameras' settings in the editor would save a transient
	// material instance into the level.
	MaterialInstance = UMaterialInstanceDynamic::Create(Base, this);
	MaterialInstance->SetScalarParameterValue(FlashParam, CurrentFlash); // a pulse in progress carries over
	BaseNoise = 0.f;
	MaterialInstance->GetScalarParameterValue(FMaterialParameterInfo(NoiseParam), BaseNoise);
	if (CurrentJam > 0.f)
	{
		MaterialInstance->SetScalarParameterValue(NoiseParam, BaseNoise + CurrentJam);
	}
	TArray<UCameraComponent*> Cameras;
	GetCameras(Cameras);
	for (UCameraComponent* Camera : Cameras)
	{
		Camera->PostProcessSettings.WeightedBlendables.Array.Add(FWeightedBlendable(bEnabled ? 1.f : 0.f, MaterialInstance));
	}
}

void UIJPCRTComponent::RemoveFromCameras()
{
	if (!MaterialInstance)
	{
		return;
	}

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
