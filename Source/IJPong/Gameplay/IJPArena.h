// It's Just Pong

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/IJPTypes.h"
#include "IJPArena.generated.h"

class UBoxComponent;
class UCameraComponent;
class UInstancedStaticMeshComponent;
class UMaterialInterface;
class AIJPPaddle;
class UIJPGoalComponent;
class UIJPSevenSegmentComponent;

/**
 * The Pong playfield: walls, goals, net, score digits and the camera that frames it.
 * Owns the mapping between arena plane space (2D, see IJPTypes.h) and world space, so the
 * whole game can be placed and oriented anywhere in a level.
 */
UCLASS()
class IJPONG_API AIJPArena : public AActor
{
	GENERATED_BODY()

public:
	AIJPArena();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// --- Plane space <-> world space ---

	UFUNCTION(BlueprintPure, Category = "Arena")
	FVector PlaneToWorld(const FVector2D& PlanePoint) const;

	UFUNCTION(BlueprintPure, Category = "Arena")
	FVector2D WorldToPlane(const FVector& WorldPoint) const;

	UFUNCTION(BlueprintPure, Category = "Arena")
	FVector PlaneDirToWorld(const FVector2D& PlaneDir) const;

	UFUNCTION(BlueprintPure, Category = "Arena")
	FVector2D WorldDirToPlane(const FVector& WorldDir) const;

	/** Inner half-size of the playfield: X = centre to goal line, Y = centre to wall's inner edge. */
	UFUNCTION(BlueprintPure, Category = "Arena")
	FVector2D GetHalfExtents() const { return HalfExtents; }

	UFUNCTION(BlueprintCallable, Category = "Arena")
	void SetScore(EIJPSide Side, int32 Score);

	/** The paddle defending this side's goal. Null before BeginPlay. */
	UFUNCTION(BlueprintPure, Category = "Arena")
	AIJPPaddle* GetPaddle(EIJPSide Side) const;

	/** Plane X of the paddle lane on this side. */
	UFUNCTION(BlueprintPure, Category = "Arena")
	float GetLaneX(EIJPSide Side) const;

	UMaterialInterface* GetPongMaterial() const { return PongMaterial; }

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena|Layout")
	FVector2D HalfExtents = FVector2D(400.f, 280.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena|Layout")
	float WallThickness = 10.f;

	/** Empty space between the walls and the screen edge. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena|Layout")
	float ScreenMargin = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena|Layout")
	bool bShowWalls = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena|Layout")
	float GoalDepth = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena|Layout")
	FVector2D NetDashSize = FVector2D(6.f, 18.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena|Layout")
	float NetDashGap = 18.f;

	/** Horizontal distance of each score from the net, and gap from the top wall. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena|Layout")
	FVector2D ScoreOffset = FVector2D(120.f, 30.f);

	/** Thickness of the visual pieces toward the camera. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena|Layout")
	float VisualDepth = 10.f;

	/** Thickness of ball-blocking volumes toward the camera; generous so small depth offsets never miss. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena|Layout")
	float BlockerDepth = 200.f;

	/** Width:height of the screen. The camera letterboxes/pillarboxes to keep it. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena|Layout")
	float ScreenAspectRatio = 4.f / 3.f;

	/** Distance from each goal line in to the centre of that side's paddle. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena|Layout")
	float PaddleInset = 40.f;

	/** Spawned for both sides at BeginPlay. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena|Paddles")
	TSubclassOf<AIJPPaddle> PaddleClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena|Look")
	TObjectPtr<UMaterialInterface> PongMaterial;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arena|Components")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arena|Components")
	TObjectPtr<UBoxComponent> TopWall;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arena|Components")
	TObjectPtr<UBoxComponent> BottomWall;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arena|Components")
	TObjectPtr<UIJPGoalComponent> LeftGoal;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arena|Components")
	TObjectPtr<UIJPGoalComponent> RightGoal;

	/** Walls and net dashes, one cube instance each. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arena|Components")
	TObjectPtr<UInstancedStaticMeshComponent> Visuals;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arena|Components")
	TObjectPtr<UIJPSevenSegmentComponent> LeftScore;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arena|Components")
	TObjectPtr<UIJPSevenSegmentComponent> RightScore;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arena|Components")
	TObjectPtr<UCameraComponent> Camera;

private:
	AIJPPaddle* SpawnPaddle(EIJPSide Side);
	FTransform GetPlaneTransform() const;
	void AddVisualBox(const FVector2D& Centre, const FVector2D& Size);

	UPROPERTY(Transient)
	TObjectPtr<AIJPPaddle> LeftPaddle;

	UPROPERTY(Transient)
	TObjectPtr<AIJPPaddle> RightPaddle;
};
