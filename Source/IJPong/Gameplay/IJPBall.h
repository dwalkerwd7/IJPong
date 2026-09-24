// It's Just Pong

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/IJPTypes.h"
#include "IJPBall.generated.h"

class AIJPArena;
class AIJPPaddle;
class UStaticMeshComponent;
struct FHitResult;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FIJPBallGoalSignature, EIJPSide, DefendingSide);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FIJPBallPaddleHitSignature, AIJPPaddle*, Paddle);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FIJPBallBounceSignature);

/**
 * The square Pong ball: a kinematic mover simulated in arena plane space.
 * It advances in fixed substeps, sweeping a box on ECC_PongBall and bouncing off whatever it hits.
 * The visible position is interpolated between the last two substeps, so it's smooth at any frame rate.
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
	void InitBall(AIJPArena* InArena);

	/**
	 * Launch from the centre toward a side at BaseSpeed. Resets the rally.
	 * @param AngleDeg  Angle from horizontal; positive goes up the screen.
	 */
	UFUNCTION(BlueprintCallable, Category = "Ball")
	void Serve(EIJPSide Toward, float AngleDeg);

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
	float GetSize() const { return Size; }

	UFUNCTION(BlueprintPure, Category = "Ball")
	float GetMaxSpeed() const { return MaxSpeed; }

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
	/** Edge length of the square ball. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball|Layout")
	float Size = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball|Layout")
	float VisualDepth = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball|Movement", meta = (ClampMin = "0"))
	float BaseSpeed = 400.f;

	/** Added to the speed on every paddle hit, up to MaxSpeed. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball|Movement", meta = (ClampMin = "0"))
	float SpeedPerHit = 25.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball|Movement", meta = (ClampMin = "0"))
	float MaxSpeed = 1000.f;

	/** Bounce angle from horizontal when the ball hits a paddle's very edge. Also the steepest the ball can ever travel. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball|Movement", meta = (ClampMin = "0", ClampMax = "85", Units = "deg"))
	float MaxBounceAngleDeg = 60.f;

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

	TWeakObjectPtr<AIJPArena> Arena;
	FVector2D Position = FVector2D::ZeroVector;
	FVector2D PreviousPosition = FVector2D::ZeroVector;
	FVector2D Velocity = FVector2D::ZeroVector;
	float Speed = 0.f;
	float Accumulator = 0.f;
	int32 RallyHits = 0;
	bool bInPlay = false;
};
