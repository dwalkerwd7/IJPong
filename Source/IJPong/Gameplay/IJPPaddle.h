// It's Just Pong

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Core/IJPTypes.h"
#include "Presentation/IJPBlinker.h"
#include "IJPPaddle.generated.h"

class AIJPArena;
class UBoxComponent;
class UIJPPaddleProfile;
class UStaticMeshComponent;

/**
 * A paddle locked to one lane of an arena. It moves only along plane Y, clamped between the walls.
 * Controllers (player or AI) feed it a -1..1 move input every frame; the paddle turns that into a
 * velocity with a very short ramp, so it feels instant but still has a real, readable velocity.
 */
UCLASS()
class IJPONG_API AIJPPaddle : public APawn
{
	GENERATED_BODY()

public:
	AIJPPaddle();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaSeconds) override;

	/**
	 * Called by the arena right after spawning. Applies the profile (null = UIJPPaddleProfile's defaults)
	 * and places the paddle in its lane, centred vertically.
	 */
	void InitPaddle(AIJPArena* InArena, EIJPSide InSide, float InLaneX, const UIJPPaddleProfile* InProfile);

	UFUNCTION(BlueprintPure, Category = "Paddle")
	const UIJPPaddleProfile* GetProfile() const;

	/** Blink off briefly: a contact cue for ball hits. */
	UFUNCTION(BlueprintCallable, Category = "Paddle")
	void Flicker();

	UFUNCTION(BlueprintPure, Category = "Paddle")
	bool IsVisualShown() const;

	/** Accumulates move input for this frame (+1 = up the screen). Consumed on the paddle's next tick. */
	UFUNCTION(BlueprintCallable, Category = "Paddle")
	void AddMoveInput(float Value);

	UFUNCTION(BlueprintPure, Category = "Paddle")
	AIJPArena* GetArena() const { return Arena.Get(); }

	UFUNCTION(BlueprintPure, Category = "Paddle")
	EIJPSide GetSide() const { return Side; }

	/** Paddle centre in arena plane space. */
	UFUNCTION(BlueprintPure, Category = "Paddle")
	FVector2D GetPlanePosition() const { return FVector2D(LaneX, PlaneY); }

	/** Current speed along plane Y (units/s, + is up). */
	UFUNCTION(BlueprintPure, Category = "Paddle")
	float GetPlaneVelocity() const { return Velocity; }

	UFUNCTION(BlueprintPure, Category = "Paddle")
	FVector2D GetSize() const;

	UFUNCTION(BlueprintPure, Category = "Paddle")
	float GetMaxSpeed() const;

	UFUNCTION(BlueprintPure, Category = "Paddle")
	float GetRampTime() const;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paddle|Layout")
	float VisualDepth = 10.f;

	/** Thickness of the ball-blocking box toward the camera; matches the arena's blockers. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paddle|Layout")
	float BlockerDepth = 200.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Paddle|Components")
	TObjectPtr<UBoxComponent> Collision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Paddle|Components")
	TObjectPtr<UStaticMeshComponent> Visual;

private:
	void ApplyLayout();
	void UpdateTransform();

	UPROPERTY(Transient)
	TObjectPtr<const UIJPPaddleProfile> Profile;

	TWeakObjectPtr<AIJPArena> Arena;
	FIJPBlinker FlickerBlinker;
	EIJPSide Side = EIJPSide::Left;
	float LaneX = 0.f;
	float PlaneY = 0.f;
	float Velocity = 0.f;
	float PendingInput = 0.f;
};
