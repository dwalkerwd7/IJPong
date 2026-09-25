// It's Just Pong

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "IJPEra.generated.h"

class UMaterialInterface;
class UIJPToneSet;

/** The parts of the screen a palette colours. */
UENUM(BlueprintType)
enum class EIJPPaletteRole : uint8
{
	Background,
	Walls,
	Net,
	Score,
	LeftPaddle,
	RightPaddle,
	Ball,
	Count UMETA(Hidden)
};

/** One colour per palette role. The defaults are the 1972 cabinet: white on black. */
USTRUCT(BlueprintType)
struct FIJPPalette
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Palette")
	FLinearColor Background = FLinearColor::Black;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Palette")
	FLinearColor Walls = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Palette")
	FLinearColor Net = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Palette")
	FLinearColor Score = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Palette")
	FLinearColor LeftPaddle = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Palette")
	FLinearColor RightPaddle = FLinearColor::White;

	/** Every ball's colour, unless bBallTypeColours shows each type's own. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Palette")
	FLinearColor Ball = FLinearColor::White;

	/** Colour each ball by its type (UIJPBallType::Colour). Off = all balls are Ball, as on a black-and-white set. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Palette")
	bool bBallTypeColours = false;

	FLinearColor Get(EIJPPaletteRole Role) const
	{
		switch (Role)
		{
		case EIJPPaletteRole::Background:  return Background;
		case EIJPPaletteRole::Walls:       return Walls;
		case EIJPPaletteRole::Net:         return Net;
		case EIJPPaletteRole::Score:       return Score;
		case EIJPPaletteRole::LeftPaddle:  return LeftPaddle;
		case EIJPPaletteRole::RightPaddle: return RightPaddle;
		case EIJPPaletteRole::Ball:        return Ball;
		default:                           return FLinearColor::White;
		}
	}
};

/**
 * One era of the game's history: how the whole game looks and sounds while it's current.
 * Eras are ordered in UIJPEraSubsystem's config; the subsystem says which one is current, and
 * everything that shows the era (arenas, CRT components) follows it live.
 */
UCLASS(BlueprintType)
class IJPONG_API UIJPEra : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Era")
	FText DisplayName;

	/** Post-process look for every CRT component that follows the era. Empty = no CRT. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Era|Look")
	TObjectPtr<UMaterialInterface> CRTMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Era|Look")
	FIJPPalette Palette;

	/** The beeps for ball events. Empty = UIJPToneSet's defaults. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Era|Sound")
	TObjectPtr<UIJPToneSet> ToneSet;
};
