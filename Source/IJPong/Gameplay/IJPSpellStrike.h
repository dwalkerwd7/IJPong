// It's Just Pong

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/IJPTypes.h"
#include "IJPSpellStrike.generated.h"

class AIJPArena;
class UStaticMeshComponent;

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
};

/**
 * One spell on its way to a paddle's lane (spawned by UIJPAbility_Spell). It marks the zone it will
 * hit (two blinking lines on the target's lane, blinking faster as it nears), then lands after
 * Delay: if the target paddle overlaps the zone it takes Damage through the match (never a goal),
 * otherwise it was dodged. A travelling spell (fireball) shows its projectile crossing the court; a
 * falling one (lightning) flashes a bolt from the top wall. Registers with the arena so the AI can
 * steer clear.
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

private:
	void Land();
	void Place(UStaticMeshComponent* Piece, const FVector2D& Centre, const FVector2D& Size) const;

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

	TWeakObjectPtr<AIJPArena> Arena;
	FIJPStrikeSpec Spec;
	EIJPSide CasterSide = EIJPSide::Left;
	EIJPSide TargetSide = EIJPSide::Right;
	float TargetY = 0.f;
	float StartX = 0.f;
	float LaneX = 0.f;
	float Elapsed = 0.f;
	float Linger = 0.f;
	bool bLanded = false;
	bool bHit = false;
};
