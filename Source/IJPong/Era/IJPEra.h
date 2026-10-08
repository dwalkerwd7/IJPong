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

/** The Arcade era's twist: targets that pop up mid-court. A ball breaks one and bounces off; if you returned it last, you get coins. */
USTRUCT(BlueprintType)
struct FIJPBonusTargets
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bonus Targets")
	bool bEnabled = false;

	/** Seconds between new targets while a ball is in play. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bonus Targets", meta = (ClampMin = "0.1", Units = "s"))
	float SpawnInterval = 6.f;

	/** Most on the court at once. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bonus Targets", meta = (ClampMin = "1"))
	int32 MaxTargets = 2;

	/** A target's side length, in arena units. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bonus Targets", meta = (ClampMin = "4"))
	float Size = 20.f;

	/** How far out from the net they can appear, as a fraction of the half-court. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bonus Targets", meta = (ClampMin = "0", ClampMax = "1"))
	float CourtFraction = 0.5f;

	/** Run coins for breaking one with a ball you returned. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bonus Targets", meta = (ClampMin = "0"))
	int32 Coins = 5;
};

/** The 16-bit era's twist: blocks that drift up and down through mid-court, turning back at the walls. Balls bounce off them. */
USTRUCT(BlueprintType)
struct FIJPDriftingBlocks
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drifting Blocks")
	bool bEnabled = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drifting Blocks", meta = (ClampMin = "1", ClampMax = "6"))
	int32 Count = 2;

	/** Width and height, in arena units. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drifting Blocks")
	FVector2D Size = FVector2D(16.f, 70.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drifting Blocks", meta = (ClampMin = "0", Units = "cm/s"))
	float Speed = 50.f;

	/** How far out from the net they sit, as a fraction of the half-court (spread evenly across it). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drifting Blocks", meta = (ClampMin = "0", ClampMax = "1"))
	float CourtFraction = 0.4f;
};

/** The Grid era's twist: every ball leaves a light trail that any ball bounces off, fading after a moment. */
USTRUCT(BlueprintType)
struct FIJPLightTrails
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light Trails")
	bool bEnabled = false;

	/** How long a piece of trail lasts. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light Trails", meta = (ClampMin = "0.05", Units = "s"))
	float Lifetime = 1.f;

	/** A piece only turns solid this long after it's laid, so a ball never hits the trail it's laying. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light Trails", meta = (ClampMin = "0", Units = "s"))
	float SolidDelay = 0.15f;

	/** The trail's thickness, in arena units. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light Trails", meta = (ClampMin = "1"))
	float Thickness = 4.f;

	/** A new piece every this many units travelled. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light Trails", meta = (ClampMin = "4"))
	float PieceLength = 24.f;
};

/** The Matrix era's twist: when a ball is about to beat a paddle, the balls slow to a crawl for a moment (paddles don't). */
USTRUCT(BlueprintType)
struct FIJPBulletTime
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bullet Time")
	bool bEnabled = false;

	/** Balls move at this fraction of their speed while it lasts. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bullet Time", meta = (ClampMin = "0.05", ClampMax = "1"))
	float TimeScale = 0.25f;

	/** How long it lasts, in real time. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bullet Time", meta = (ClampMin = "0", Units = "s"))
	float Duration = 0.7f;

	/** It starts when a ball will reach the paddle's lane within this long, where the paddle isn't. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bullet Time", meta = (ClampMin = "0", Units = "s"))
	float Lead = 0.2f;
};

/** The HD era's twist: between points the court changes size, smoothly, within limits. */
USTRUCT(BlueprintType)
struct FIJPCourtShift
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Court Shift")
	bool bEnabled = false;

	/** Smallest court, as a fraction of the full size (width, height). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Court Shift")
	FVector2D MinScale = FVector2D(0.75f, 0.7f);

	/** Largest court (1 = full size, the screen's frame). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Court Shift")
	FVector2D MaxScale = FVector2D(1.f, 1.f);

	/** How long the reshape takes. Keep it under the serve delay, so the court has settled before play. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Court Shift", meta = (ClampMin = "0", Units = "s"))
	float ShiftTime = 0.6f;
};

/** The Fighting game era's twist: consecutive returns fill a combo meter; full, the next return is a super shot. */
USTRUCT(BlueprintType)
struct FIJPCombo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combo")
	bool bEnabled = false;

	/** Returns in a row (without a goal) that fill a side's meter. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combo", meta = (ClampMin = "1"))
	int32 ReturnsToFill = 6;

	/** The super shot leaves this many times faster... */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combo", meta = (ClampMin = "1"))
	float SuperBoost = 2.f;

	/** ...and deals this many times the damage if it scores. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combo", meta = (ClampMin = "1"))
	float SuperDamage = 2.f;
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

	/** Targets popping up mid-court (the Arcade twist). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Era|Twist")
	FIJPBonusTargets BonusTargets;

	/** Matches open fighting-game style: "YOU VS <RIVAL>", the rival's pre-match lines, then "ROUND 1", "FIGHT!". */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Era|Look")
	bool bVersusIntro = false;

	/** Combo meters and super shots (the Fighting game twist). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Era|Twist")
	FIJPCombo Combo;

	/** The court reshaping between points (the HD twist). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Era|Twist")
	FIJPCourtShift CourtShift;

	/** Bullet time on near-goals (the Matrix twist). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Era|Twist")
	FIJPBulletTime BulletTime;

	/** Light trails behind every ball (the Grid twist). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Era|Twist")
	FIJPLightTrails LightTrails;

	/** Bloom on the arena camera, so bright colours glow (eras past the CRT). 0 = none. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Era|Look", meta = (ClampMin = "0"))
	float BloomIntensity = 0.f;

	/** Blocks drifting through mid-court (the 16-bit twist). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Era|Twist")
	FIJPDriftingBlocks DriftingBlocks;

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
