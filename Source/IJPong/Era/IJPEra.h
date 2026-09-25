// It's Just Pong

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "IJPEra.generated.h"

class UIJPActConfig;
class UMaterialInterface;
class UIJPToneSet;
class UIJPBackdrop;
class UTexture2D;

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

/**
 * An era's health bar art: a frame and the fill that sits inside it, both greyscale and drawn for
 * the LEFT side (the right one is mirrored). Sizes come from the textures at PixelsPerUnit.
 */
USTRUCT(BlueprintType)
struct FIJPHealthBarStyle
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Health Bar")
	TObjectPtr<UTexture2D> Frame;

	/** Drawn full width at full health and cut from the net side as health drops. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Health Bar")
	TObjectPtr<UTexture2D> Fill;

	/** Where the fill's top-left corner sits in the frame texture, in pixels. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Health Bar")
	FVector2D FillOffset = FVector2D::ZeroVector;

	/** Segmented fills: how many segments, and their pitch in pixels, so the cut lands between them. 0 = smooth. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Health Bar", meta = (ClampMin = "0"))
	int32 Segments = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Health Bar", meta = (ClampMin = "0", EditCondition = "Segments > 0"))
	float SegmentPitch = 0.f;

	/** Texture pixels per game unit (the sprite brief exports at 8x). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Health Bar", meta = (ClampMin = "0.1"))
	float PixelsPerUnit = 8.f;

	bool IsSet() const { return Frame && Fill; }
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

	/** Paddles and balls are drawn with their sprites (tinted by the palette). Off = plain rectangles (1972). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Era|Look")
	bool bShowSprites = false;

	/** Balls use their early, square "classic" sprite where they have one (the early sprite eras). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Era|Look", meta = (EditCondition = "bShowSprites"))
	bool bClassicBallSprites = false;

	/**
	 * How quickly rivals react in this era, as a multiple of their AI profile's reaction time at the
	 * fight's skill (1 = as tuned; 0.75 = a quarter quicker). Later eras react faster.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Era|Rivals", meta = (ClampMin = "0.1", ClampMax = "2"))
	float AIReactionScale = 1.f;

	/** Scenery behind the court (eras with sprites): one picked at random for each fight. Empty = a plain screen. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Era|Look", meta = (EditCondition = "bShowSprites"))
	TArray<TObjectPtr<UIJPBackdrop>> Backdrops;

	/** Health as bars instead of seven-segment numbers (eras with sprites only; unset = keep the numbers). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Era|Look", meta = (EditCondition = "bShowSprites"))
	FIJPHealthBarStyle HealthBar;

	/** Shown when a run climbs into this era (e.g. "1978 - ARCADE"). Empty = DisplayName. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Era|Run")
	FText TitleCard;

	/** The era's acts, in order. A run plays all of them while this is your newest era. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Era|Run")
	TArray<TObjectPtr<UIJPActConfig>> Acts;

	/** Once this era is beaten, how many of its acts (its first ones) a run passes through. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Era|Run", meta = (ClampMin = "1"))
	int32 ActsWhenBeaten = 1;

	/** Chat-bubble corner radius, in arena units. 0 = square corners (hardware that can't draw curves). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Era|Look", meta = (ClampMin = "0"))
	float BubbleCornerRadius = 0.f;

	/** Chat-bubble tail: a smooth wedge (true) or a staircase of pixel steps (false). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Era|Look")
	bool bSmoothBubbleTail = false;

	/** The beeps for ball events. Empty = UIJPToneSet's defaults. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Era|Sound")
	TObjectPtr<UIJPToneSet> ToneSet;
};
