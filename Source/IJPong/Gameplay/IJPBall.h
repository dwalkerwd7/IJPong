// It's Just Pong

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/IJPTypes.h"
#include "Presentation/IJPBlinker.h"
#include "IJPBall.generated.h"

class AIJPArena;
class AIJPBall;
class AIJPPaddle;
class UIJPBallType;
class UMaterialInstanceDynamic;
class UStaticMeshComponent;
struct FHitResult;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FIJPBallGoalSignature, AIJPBall*, Ball, EIJPSide, DefendingSide);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FIJPBallPaddleHitSignature, AIJPBall*, Ball, AIJPPaddle*, Paddle);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FIJPBallBounceSignature);

/**
 * The square Pong ball: a kinematic mover simulated in arena plane space.
 * It advances in fixed substeps, sweeping a box on ECC_PongBall and bouncing off whatever it hits.
 * The visible position is interpolated between the last two substeps, so it's smooth at any frame rate.
 * Its size, speeds and points come from its UIJPBallType; the arena may have several in play.
 */
UCLASS()
class IJPONG_API AIJPBall : public AActor
{
	GENERATED_BODY()

public:
	AIJPBall();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaSeconds) override;

	/** Called by the arena right after spawning. The ball waits, hidden, at the centre until served. */
	void InitBall(AIJPArena* InArena, const UIJPBallType* InType);

	/** Become another kind of ball. Takes effect at once; a ball in play keeps its current speed until its next hit. Null = UIJPBallType's defaults. */
	UFUNCTION(BlueprintCallable, Category = "Ball")
	void SetType(const UIJPBallType* InType);

	/** The type in use: the one set, or UIJPBallType's defaults. */
	const UIJPBallType& GetType() const;

	/** Re-read this ball's colour from the arena (its era palette and this ball's type). */
	void RefreshColour();

	/**
	 * Launch from the centre toward a side at BaseSpeed. Resets the rally.
	 * @param AngleDeg  Angle from horizontal; positive goes up the screen.
	 */
	UFUNCTION(BlueprintCallable, Category = "Ball")
	void Serve(EIJPSide Toward, float AngleDeg);

	/** Wait at the centre, blinking (starting hidden), until Serve() launches it. */
	UFUNCTION(BlueprintCallable, Category = "Ball")
	void BlinkAtCentre();

	UFUNCTION(BlueprintPure, Category = "Ball")
	bool IsBlinking() const { return ServeBlinker.IsRunning(); }

	/**
	 * Speed the ball up by Multiplier for this one shot: the next paddle hit carries on from the
	 * speed it had before the boost, as if it never happened. For abilities like Smash.
	 */
	UFUNCTION(BlueprintCallable, Category = "Ball")
	void Boost(float Multiplier);

	/** Take the ball out of play and hide it at the centre. */
	UFUNCTION(BlueprintCallable, Category = "Ball")
	void ResetBall();

	UFUNCTION(BlueprintPure, Category = "Ball")
	bool IsInPlay() const { return bInPlay; }

	/** Simulated centre in plane space (not the interpolated, drawn one). */
	UFUNCTION(BlueprintPure, Category = "Ball")
	FVector2D GetPlanePosition() const { return Position; }

	UFUNCTION(BlueprintPure, Category = "Ball")
	FVector2D GetPlaneVelocity() const { return Velocity; }

	/** Paddle hits since the last serve. */
	UFUNCTION(BlueprintPure, Category = "Ball")
	int32 GetRallyHits() const { return RallyHits; }

	UFUNCTION(BlueprintPure, Category = "Ball")
	float GetSize() const;

	UFUNCTION(BlueprintPure, Category = "Ball")
	float GetMaxSpeed() const;

	UFUNCTION(BlueprintPure, Category = "Ball")
	float GetMaxBounceAngle() const { return MaxBounceAngleDeg; }

	/** The ball entered a goal box. DefendingSide concedes the point. */
	UPROPERTY(BlueprintAssignable, Category = "Ball")
	FIJPBallGoalSignature OnGoal;

	UPROPERTY(BlueprintAssignable, Category = "Ball")
	FIJPBallPaddleHitSignature OnPaddleHit;

	/** A plain mirror bounce: walls, and the top/bottom edges of paddles. */
	UPROPERTY(BlueprintAssignable, Category = "Ball")
	FIJPBallBounceSignature OnBounce;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball|Layout")
	float VisualDepth = 10.f;

	/** Bounce angle from horizontal when the ball hits a paddle's very edge. Also the steepest the ball can ever travel. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball|Movement", meta = (ClampMin = "0", ClampMax = "85", Units = "deg"))
	float MaxBounceAngleDeg = 60.f;

	/** Seconds between on/off toggles while waiting to be served. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball|Presentation", meta = (ClampMin = "0.01", Units = "s"))
	float ServeBlinkPeriod = 0.15f;

	/** Simulation substeps per second. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball|Simulation", meta = (ClampMin = "30"))
	float SimRate = 120.f;

	/** Surfaces the ball may bounce off within one substep before the rest of that substep is dropped. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball|Simulation", meta = (ClampMin = "1"))
	int32 MaxBouncesPerStep = 4;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ball|Components")
	TObjectPtr<UStaticMeshComponent> Visual;

private:
	void Substep(float StepSeconds);
	bool Sweep(const FVector2D& From, const FVector2D& To, FHitResult& OutHit) const;
	void HandleHit(const FHitResult& Hit);
	bool TryPaddleBounce(AIJPPaddle* Paddle, const FVector2D& Normal);
	void UpdateDrawnTransform(float Alpha);
	void ApplySize();

	TWeakObjectPtr<AIJPArena> Arena;

	UPROPERTY(Transient)
	TObjectPtr<const UIJPBallType> Type;

	/** This ball's own copy of the arena's material, so each ball can show its type's colour. */
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> ColourMaterial;

	FIJPBlinker ServeBlinker;
	FVector2D Position = FVector2D::ZeroVector;
	FVector2D PreviousPosition = FVector2D::ZeroVector;
	FVector2D Velocity = FVector2D::ZeroVector;
	float Speed = 0.f;
	/** The speed before a Boost, which the next paddle hit builds on. 0 = not boosted. */
	float UnboostedSpeed = 0.f;
	float Accumulator = 0.f;
	int32 RallyHits = 0;
	bool bInPlay = false;
};
