// It's Just Pong

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "IJPBackdrop.generated.h"

class UTexture2D;

/**
 * One layer of a backdrop. Placement and motion are in screen fractions (0..1 across the 4:3
 * screen, from its top-left), so a layer fits any screen size.
 */
USTRUCT(BlueprintType)
struct FIJPBackdropLayer
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Layer")
	TObjectPtr<UTexture2D> Texture;

	/** Swapped in when the backdrop breaks (a boss's crumbling phase). Empty = no change. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Layer")
	TObjectPtr<UTexture2D> BrokenTexture;

	/** Where its centre sits (0,0 = top-left, 1,1 = bottom-right). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Layer|Placement")
	FVector2D Centre = FVector2D(0.5f, 0.5f);

	/** Its size as a fraction of the screen (1,1 = fills it). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Layer|Placement")
	FVector2D Size = FVector2D(1.f, 1.f);

	/** More of the same sprite at these centres (e.g. a row of torches). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Layer|Placement")
	TArray<FVector2D> MoreCentres;

	/** Scrolls its picture by this much of the screen per second (+Y = up); it wraps, so the texture must tile. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Layer|Motion")
	FVector2D Drift = FVector2D::ZeroVector;

	/** How far its brightness flickers (0 = steady, 1 = down to black). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Layer|Motion", meta = (ClampMin = "0", ClampMax = "1"))
	float Flicker = 0.f;

	/** Flickers per second, roughly (it wavers rather than blinking). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Layer|Motion", meta = (ClampMin = "0"))
	float FlickerSpeed = 6.f;

	/** How much a shake moves it (near layers more than far ones). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Layer|Motion", meta = (ClampMin = "0"))
	float ShakeScale = 1.f;

	/** Its drift is this many times faster for a moment after the backdrop breaks (a burst of falling debris). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Layer|Motion", meta = (ClampMin = "1"))
	float BreakBurst = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Layer")
	FLinearColor Tint = FLinearColor::White;
};

/** Scenery behind the court (eras with sprites): layers drawn back to front, first = furthest. */
UCLASS(BlueprintType)
class IJPONG_API UIJPBackdrop : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Backdrop")
	TArray<FIJPBackdropLayer> Layers;
};
