// It's Just Pong

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "IJPCRTComponent.generated.h"

class UCameraComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;

/**
 * Puts the CRT post-process look on every camera of the actor it's added to: the arena, a plain
 * CameraActor, a menu backdrop camera, a CineCamera... Applied at BeginPlay, removed at EndPlay.
 * Each component gets its own dynamic material instance, so its look can be changed at runtime.
 */
UCLASS(Config = Game, ClassGroup = (IJPong), meta = (BlueprintSpawnableComponent))
class IJPONG_API UIJPCRTComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UIJPCRTComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Brief brightness pulse of the whole screen, fading out over Duration. */
	UFUNCTION(BlueprintCallable, Category = "CRT")
	void Pulse(float Strength = 1.f, float Duration = 0.35f);

	/** Current value of the material's Flash parameter (0 when not pulsing). */
	UFUNCTION(BlueprintPure, Category = "CRT")
	float GetFlash() const { return CurrentFlash; }

	/** Turn the look on or off without removing the component. */
	UFUNCTION(BlueprintCallable, Category = "CRT")
	void SetCRTEnabled(bool bEnable);

	UFUNCTION(BlueprintPure, Category = "CRT")
	bool IsCRTEnabled() const { return bEnabled; }

	/** The live material, for runtime tweaks (e.g. SetScalarParameterValue("Curvature", ...)). Null before BeginPlay. */
	UFUNCTION(BlueprintPure, Category = "CRT")
	UMaterialInstanceDynamic* GetCRTMaterial() const { return MaterialInstance; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Post-process material for the look. Default from DefaultGame.ini; can be overridden per component. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "CRT")
	TSoftObjectPtr<UMaterialInterface> CRTMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CRT")
	bool bEnabled = true;

private:
	void SetFlash(float Value);
	void ApplyWeight(float Weight);
	void GetCameras(TArray<UCameraComponent*>& OutCameras) const;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> MaterialInstance;

	float PulseStrength = 0.f;
	float PulseDuration = 0.f;
	float PulseElapsed = 0.f;
	float CurrentFlash = 0.f;
};
