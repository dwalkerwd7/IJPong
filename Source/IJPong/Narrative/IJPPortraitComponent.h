// It's Just Pong

#pragma once

#include "CoreMinimal.h"
#include "Components/StaticMeshComponent.h"
#include "Narrative/IJPPortrait.h"
#include "IJPPortraitComponent.generated.h"

class UMaterialInstanceDynamic;
class UMaterialInterface;

/**
 * One side's character portrait by its health (eras with sprites): a flat quad facing the camera,
 * showing the face a spoken line asks for, or else the match mood (smug ahead, rattled behind).
 * Centred on its origin in the arena's X / Z plane.
 */
UCLASS(ClassGroup = (IJPong), meta = (BlueprintSpawnableComponent))
class IJPONG_API UIJPPortraitComponent : public UStaticMeshComponent
{
	GENERATED_BODY()

public:
	UIJPPortraitComponent();

	/** Whose faces to show (unset = none). */
	void SetPortraits(const FIJPPortraits& InPortraits);

	/** Draw it (a sprite era, with this material) or not, in this colour. */
	void SetLook(bool bSpriteEra, UMaterialInterface* SpriteMaterial, const FLinearColor& Colour);

	/** The match's mood for this side (never Mood itself). */
	void SetMood(EIJPExpression InMood);

	/** The face a line being spoken asks for; Mood = none (back to the match mood). */
	void SetLineExpression(EIJPExpression InExpression);

	/** The face showing now (Neutral / Smug / Rattled), whether or not it's drawn. */
	UFUNCTION(BlueprintPure, Category = "Portrait")
	EIJPExpression GetExpression() const { return LineExpression != EIJPExpression::Mood ? LineExpression : Mood; }

	UFUNCTION(BlueprintPure, Category = "Portrait")
	bool IsFaceShown() const { return IsVisible(); }

	/** Width and height in units. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Portrait")
	float Size = 48.f;

private:
	void Refresh();

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> Material;

	FIJPPortraits Portraits;
	EIJPExpression Mood = EIJPExpression::Neutral;
	EIJPExpression LineExpression = EIJPExpression::Mood;
	bool bSpriteEra = false;
};
