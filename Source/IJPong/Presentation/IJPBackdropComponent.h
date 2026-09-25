// It's Just Pong

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "IJPBackdropComponent.generated.h"

class UIJPBackdrop;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UStaticMeshComponent;

/**
 * Draws a backdrop's layers across the whole screen, behind every piece of the court and the chat
 * bubbles, and keeps it gently alive: drifting layers, flickering lights, a tremor on big hits,
 * and a break (swapped layers plus a burst) for a boss's crumbling phase. Centred on its origin
 * in the arena's X / Z plane.
 */
UCLASS(ClassGroup = (IJPong), meta = (BlueprintSpawnableComponent))
class IJPONG_API UIJPBackdropComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	UIJPBackdropComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Show this backdrop (null = none) across a screen this big, drawn with Material. Starts unbroken. */
	void SetBackdrop(const UIJPBackdrop* InBackdrop, UMaterialInterface* Material, const FVector2D& InScreenSize);

	/** Draw it or not (eras without sprites have no backdrops). */
	void SetShown(bool bInShown);

	UFUNCTION(BlueprintPure, Category = "Backdrop")
	bool IsShown() const { return bShown && Backdrop != nullptr; }

	const UIJPBackdrop* GetBackdrop() const { return Backdrop; }

	/** A short tremor; Strength scales ShakeAmount. */
	UFUNCTION(BlueprintCallable, Category = "Backdrop")
	void Shake(float Strength = 1.f);

	/** Swap in the broken layers, with a hard shake and each layer's burst. Once per backdrop. */
	UFUNCTION(BlueprintCallable, Category = "Backdrop")
	void Break();

	UFUNCTION(BlueprintPure, Category = "Backdrop")
	bool IsBroken() const { return bBroken; }

	UFUNCTION(BlueprintPure, Category = "Backdrop")
	bool IsShaking() const { return ShakeLeft > 0.f; }

	/** How far layer Index has scrolled, in screen fractions. */
	FVector2D GetScroll(int32 Index) const { return Scroll.IsValidIndex(Index) ? Scroll[Index] : FVector2D::ZeroVector; }

	/** The texture layer Index is showing now. */
	const UTexture2D* GetShownTexture(int32 Index) const;

	/** Units a full-strength shake moves a layer with ShakeScale 1. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Backdrop", meta = (ClampMin = "0"))
	float ShakeAmount = 5.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Backdrop", meta = (ClampMin = "0", Units = "s"))
	float ShakeTime = 0.35f;

	/** Shake strength when it breaks. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Backdrop", meta = (ClampMin = "0"))
	float BreakShake = 3.f;

	/** Seconds of each layer's BreakBurst. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Backdrop", meta = (ClampMin = "0", Units = "s"))
	float BurstTime = 1.2f;

private:
	struct FPiece
	{
		int32 Layer = 0;
		FVector2D Centre = FVector2D::ZeroVector;
	};

	void Rebuild(UMaterialInterface* Material);
	void UpdatePieces();
	void ApplyTextures();

	UPROPERTY(Transient)
	TObjectPtr<const UIJPBackdrop> Backdrop;

	/** One quad per sprite placed (a layer with MoreCentres has several), made as needed. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> Quads;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> Materials;

	TArray<FPiece> Pieces;
	TArray<FVector2D> Scroll;
	FVector2D ScreenSize = FVector2D(800.f, 600.f);
	FVector2D ShakeOffset = FVector2D::ZeroVector;
	float Time = 0.f;
	float ShakeLeft = 0.f;
	float ShakeStrength = 0.f;
	float BurstLeft = 0.f;
	bool bShown = false;
	bool bBroken = false;
};
