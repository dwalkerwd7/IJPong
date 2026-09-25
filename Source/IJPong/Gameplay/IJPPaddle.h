// It's Just Pong

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Core/IJPTypes.h"
#include "Presentation/IJPBlinker.h"
#include "IJPPaddle.generated.h"

class AIJPArena;
class UBoxComponent;
class UIJPAbilityComponent;
class UIJPSpeechBubbleComponent;
class UIJPPaddleClass;
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
	 * Called by the arena right after spawning. Applies the class (null = default stats, no skill)
	 * and places the paddle in its lane, centred vertically.
	 */
	void InitPaddle(AIJPArena* InArena, EIJPSide InSide, float InLaneX, const UIJPPaddleClass* InClass);

	/** Become another class now: its stats and class skill. Stays in its lane, inside the walls. */
	UFUNCTION(BlueprintCallable, Category = "Paddle")
	void SetPaddleClass(const UIJPPaddleClass* InClass);

	/** Null = no class (default stats). */
	UFUNCTION(BlueprintPure, Category = "Paddle")
	const UIJPPaddleClass* GetPaddleClass() const { return PaddleClass; }

	/** Base stats: the class's profile, or UIJPPaddleProfile's defaults. */
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

	/**
	 * Burst Distance along the lane over Duration, ignoring MaxSpeed and input, in the direction the
	 * paddle is being steered (or last was). Stops early at a wall. For abilities like Dash.
	 */
	UFUNCTION(BlueprintCallable, Category = "Paddle")
	void Dash(float Distance, float Duration);

	UFUNCTION(BlueprintPure, Category = "Paddle")
	bool IsDashing() const { return DashTimeLeft > 0.f; }

	/** Stretch the paddle's length (1 = the profile's). Stays inside the walls. For abilities like Grow. */
	UFUNCTION(BlueprintCallable, Category = "Paddle")
	void SetLengthScale(float Scale);

	UFUNCTION(BlueprintPure, Category = "Paddle")
	UIJPAbilityComponent* GetAbilities() const { return Abilities; }

	UFUNCTION(BlueprintPure, Category = "Paddle")
	UIJPSpeechBubbleComponent* GetSpeechBubble() const { return SpeechBubble; }

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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Paddle|Components")
	TObjectPtr<UIJPAbilityComponent> Abilities;

	/** What this paddle says (rival banter, the player's replies). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Paddle|Components")
	TObjectPtr<UIJPSpeechBubbleComponent> SpeechBubble;

private:
	void ApplyLayout();
	void UpdateTransform();
	/** Keep the paddle between the walls. True if it had to move. */
	bool ClampToWalls();

	UPROPERTY(Transient)
	TObjectPtr<const UIJPPaddleClass> PaddleClass;

	TWeakObjectPtr<AIJPArena> Arena;
	FIJPBlinker FlickerBlinker;
	EIJPSide Side = EIJPSide::Left;
	float LaneX = 0.f;
	float PlaneY = 0.f;
	float Velocity = 0.f;
	float PendingInput = 0.f;
	float LengthScale = 1.f;
	/** +1 or -1: the way the paddle was last steered. */
	float LastMoveSign = 1.f;
	float DashVelocity = 0.f;
	float DashTimeLeft = 0.f;
};
