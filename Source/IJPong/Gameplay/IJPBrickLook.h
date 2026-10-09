// It's Just Pong

#pragma once

#include "CoreMinimal.h"
#include "IJPBrickLook.generated.h"

/**
 * A paddle drawn as a chunky brick wall (M_PongBricks) in eras without sprites: some of a boss's
 * art, kept as plain as the era. Bricks keep their size, so a shrinking paddle loses rows rather
 * than squashing them.
 */
USTRUCT(BlueprintType)
struct FIJPBrickLook
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bricks")
	bool bEnabled = false;

	/** A brick's height along the paddle (rows are evened out to fit the length). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bricks", meta = (ClampMin = "2", EditCondition = "bEnabled"))
	float BrickLength = 18.f;

	/** The dark gap between bricks. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bricks", meta = (ClampMin = "0", EditCondition = "bEnabled"))
	float Mortar = 3.f;

	/** A solid cap with two bolt holes at each end (the sprite's bolted ends). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bricks", meta = (EditCondition = "bEnabled"))
	bool bBolts = true;
};
