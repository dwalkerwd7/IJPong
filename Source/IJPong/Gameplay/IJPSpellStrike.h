// It's Just Pong

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/IJPTypes.h"
#include "IJPSpellStrike.generated.h"

class AIJPArena;
class UMaterialInstanceDynamic;
class UStaticMeshComponent;
class UTexture2D;

/** How a spell looks on its way in. */
USTRUCT(BlueprintType)
struct FIJPStrikeSpec
{
	GENERATED_BODY()

	/** Seconds from the cast to the hit: the warning (and, for a travelling spell, its flight). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Strike", meta = (ClampMin = "0.05", Units = "s"))
	float Delay = 0.35f;

	/** Half the height of the zone it hits on the lane. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Strike", meta = (ClampMin = "1"))
	float HalfHeight = 20.f;

	/** Health taken if the paddle is in the zone when it lands. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Strike", meta = (ClampMin = "0"))
	float Damage = 0.5f;

	/** A projectile crosses the court to the zone (a fireball); otherwise it strikes from above (lightning). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Strike")
	bool bTravels = false;

	/** Falls from the top wall onto the zone over the whole Delay (Brickfall), instead of striking at the end. Ignored when it travels. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Strike", meta = (EditCondition = "!bTravels"))
	bool bFalls = false;

	/**
	 * In eras that show sprites: the projectile, the falling piece, or the bolt (stretched from the top
	 * wall down to the zone, ShotSize.X wide). Drawn for a shot heading right; mirrored the other way.
	 * Empty = the plain shape every era without sprites uses.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Strike|Sprites")
	TObjectPtr<UTexture2D> ShotSprite;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Strike|Sprites")
	FVector2D ShotSize = FVector2D(24.f, 24.f);

	/** In sprite eras: the target marker, stretched over the zone in place of the two lines. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Strike|Sprites")
	TObjectPtr<UTexture2D> MarkerSprite;

	/** The marker's drawn size, centred on the zone. A zero height fits it to the zone (width from the art's proportions). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Strike|Sprites")
	FVector2D MarkerSize = FVector2D::ZeroVector;

	/** In sprite eras: shown where it lands for ImpactTime (Brickfall's dust). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Strike|Sprites")
	TObjectPtr<UTexture2D> ImpactSprite;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Strike|Sprites")
	FVector2D ImpactSize = FVector2D(40.f, 20.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Strike|Sprites", meta = (ClampMin = "0", Units = "s"))
	float ImpactTime = 0.35f;
};

/**
 * One spell on its way to a paddle's lane (spawned by UIJPAbility_Spell). It marks the zone it will
 * hit (two blinking lines on the target's lane, blinking faster as it nears), then lands after
 * Delay: if the target paddle overlaps the zone it takes Damage through the match (never a goal),
 * otherwise it was dodged. A travelling spell (fireball) shows its projectile crossing the court, a
 * falling one (Brickfall) drops from the top wall onto the zone, and the rest (lightning) flash a
 * bolt from the top wall as they land. In sprite eras each piece can be art from the spec (shot,
 * marker, impact). Registers with the arena so the AI can steer clear.
 */
UCLASS()
class IJPONG_API AIJPSpellStrike : public AActor
{
	GENERATED_BODY()

public:
	AIJPSpellStrike();

	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Aim at TargetSide's lane at plane height Y, cast from CasterSide. Call right after spawning. */
	void Launch(AIJPArena* InArena, EIJPSide InCasterSide, EIJPSide InTargetSide, float InY, const FIJPStrikeSpec& InSpec);

	EIJPSide GetTargetSide() const { return TargetSide; }
	float GetTargetY() const { return TargetY; }
	float GetHalfHeight() const { return Spec.HalfHeight; }
	float GetTimeLeft() const { return FMath::Max(Spec.Delay - Elapsed, 0.f); }
	bool HasLanded() const { return bLanded; }

	/** Whether it hit (only meaningful once landed). */
	bool DidHit() const { return bHit; }

	/** For tests: the drawn pieces. */
	bool IsShotSpriteShown() const;
	bool IsMarkerSpriteShown() const;
	bool IsImpactShown() const;
	/** The shot's plane position now (the projectile, the falling piece, or the bolt's middle). */
	FVector2D GetShotPosition() const { return ShotPosition; }

private:
	void Land();
	void Place(UStaticMeshComponent* Piece, const FVector2D& Centre, const FVector2D& Size) const;
	/** Draw the shot (the sprite if there is one, else the box) centred at Centre. */
	void PlaceShot(const FVector2D& Centre, const FVector2D& Size);
	/** Give Quad a sprite material showing Sprite in Colour; false (and Quad stays hidden) if this era draws no sprites. */
	bool PrepareSprite(UStaticMeshComponent* Quad, UTexture2D* Sprite, const FLinearColor& Colour);
	void ShowMarker(bool bShow);

	UPROPERTY(VisibleAnywhere, Category = "Strike")
	TObjectPtr<USceneComponent> Root;

	/** Two lines bracketing the zone on the lane. */
	UPROPERTY(VisibleAnywhere, Category = "Strike")
	TObjectPtr<UStaticMeshComponent> MarkerTop;

	UPROPERTY(VisibleAnywhere, Category = "Strike")
	TObjectPtr<UStaticMeshComponent> MarkerBottom;

	/** The fireball, or the lightning bolt. */
	UPROPERTY(VisibleAnywhere, Category = "Strike")
	TObjectPtr<UStaticMeshComponent> Shot;

	/** Sprite-era stand-ins for the pieces above, plus the impact. */
	UPROPERTY(VisibleAnywhere, Category = "Strike")
	TObjectPtr<UStaticMeshComponent> ShotQuad;

	UPROPERTY(VisibleAnywhere, Category = "Strike")
	TObjectPtr<UStaticMeshComponent> MarkerQuad;

	UPROPERTY(VisibleAnywhere, Category = "Strike")
	TObjectPtr<UStaticMeshComponent> ImpactQuad;

	TWeakObjectPtr<AIJPArena> Arena;
	FIJPStrikeSpec Spec;
	EIJPSide CasterSide = EIJPSide::Left;
	EIJPSide TargetSide = EIJPSide::Right;
	float TargetY = 0.f;
	float StartX = 0.f;
	float LaneX = 0.f;
	float Elapsed = 0.f;
	float Linger = 0.f;
	FVector2D ShotPosition = FVector2D::ZeroVector;
	bool bLanded = false;
	bool bHit = false;
	bool bShotSprite = false;
	bool bMarkerSprite = false;
	bool bImpactSprite = false;
};
