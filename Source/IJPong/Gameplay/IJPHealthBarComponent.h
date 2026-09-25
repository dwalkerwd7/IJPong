// It's Just Pong

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "Era/IJPEra.h"
#include "Presentation/IJPBlinker.h"
#include "IJPHealthBarComponent.generated.h"

class UMaterialInstanceDynamic;
class UMaterialInterface;
class UStaticMeshComponent;

/**
 * One side's health as an era's bar art (eras with sprites): the frame, the fill cut to the health
 * left, and a dim trail behind it that holds the lost chunk for a moment before draining after it.
 * Laid out for the LEFT side in its own X / Z plane: the origin is the frame's outer (left) edge at
 * its vertical centre, and it grows toward +X. The arena mirrors the right side's bar with a
 * negative X scale, so both drain toward the net.
 */
UCLASS(ClassGroup = (IJPong), meta = (BlueprintSpawnableComponent))
class IJPONG_API UIJPHealthBarComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	UIJPHealthBarComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Use this art (drawn with the sprite material), or hide the bar when the style isn't set. */
	void SetStyle(const FIJPHealthBarStyle& InStyle, UMaterialInterface* SpriteMaterial);

	void SetColours(const FLinearColor& FrameColour, const FLinearColor& FillColour);

	/** Health left as a fraction of the most. A drop leaves the trail behind (unless bInstant); a rise takes it along. */
	UFUNCTION(BlueprintCallable, Category = "Health Bar")
	void SetFraction(float InFraction, bool bInstant = false);

	/** Show or hide the whole bar (the arena shows numbers instead when the era has no bar). */
	void SetShown(bool bInShown);

	UFUNCTION(BlueprintPure, Category = "Health Bar")
	bool IsShown() const { return bShown; }

	UFUNCTION(BlueprintPure, Category = "Health Bar")
	float GetFraction() const { return Fraction; }

	UFUNCTION(BlueprintPure, Category = "Health Bar")
	float GetTrailFraction() const { return TrailFraction; }

	/** The fill's cut across its texture (0..1), snapped to whole segments on segmented fills. */
	float GetFillCut() const { return CutFor(Fraction); }

	/** Width and height of the frame, in units. */
	FVector2D GetSize() const;

	/** Blink the whole bar NumFlashes times; 0 = until StopFlash() (the winner). */
	void Flash(int32 NumFlashes, float Period);
	void StopFlash();

	UFUNCTION(BlueprintPure, Category = "Health Bar")
	bool IsFlashing() const { return Blinker.IsRunning(); }

	/** Seconds the lost chunk holds before the trail drains after it. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Health Bar", meta = (ClampMin = "0", Units = "s"))
	float TrailHold = 0.5f;

	/** How fast the trail drains, in whole bars per second. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Health Bar", meta = (ClampMin = "0.01"))
	float TrailSpeed = 1.f;

	/** The trail's brightness, as a fraction of the fill's colour. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Health Bar", meta = (ClampMin = "0", ClampMax = "1"))
	float TrailDim = 0.35f;

private:
	UStaticMeshComponent* MakePiece(const TCHAR* Name, UMaterialInterface* SpriteMaterial, UMaterialInstanceDynamic*& OutMaterial);
	void Layout();
	void UpdateCuts();
	float CutFor(float InFraction) const;
	void ApplyVisibility(bool bFlashShow);

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> FramePiece;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> TrailPiece;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> FillPiece;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> FrameMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> TrailMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> FillMaterial;

	FIJPHealthBarStyle Style;
	FIJPBlinker Blinker;
	float Fraction = 1.f;
	float TrailFraction = 1.f;
	float TrailWait = 0.f;
	bool bShown = false;
};
