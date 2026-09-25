// It's Just Pong

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/IJPTypes.h"
#include "Era/IJPEra.h"
#include "IJPArena.generated.h"

class UBoxComponent;
class UCameraComponent;
class UInstancedStaticMeshComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UStaticMeshComponent;
class AIJPBall;
class AIJPPaddle;
class UIJPPaddleProfile;
class UIJPCRTComponent;
class UIJPGoalComponent;
class UIJPSevenSegmentComponent;
class UIJPToneSet;
class UIJPToneSynthComponent;
class UIJPEra;

/**
 * The Pong playfield: walls, goals, net, score digits and the camera that frames it.
 * Owns the mapping between arena plane space (2D, see IJPTypes.h) and world space, so the
 * whole game can be placed and oriented anywhere in a level.
 * Coloured and voiced by the current era (UIJPEraSubsystem), following era changes live.
 */
UCLASS(Config = Game)
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

	/** Flash one side's score digits. */
	UFUNCTION(BlueprintCallable, Category = "Arena")
	void FlashScore(EIJPSide Side);

	/** The match is over: the winner's score blinks until ClearWinner(). */
	UFUNCTION(BlueprintCallable, Category = "Arena")
	void ShowWinner(EIJPSide Winner);

	/** Stop showing a winner (a new match is starting). */
	UFUNCTION(BlueprintCallable, Category = "Arena")
	void ClearWinner();

	UIJPSevenSegmentComponent* GetScoreDisplay(EIJPSide Side) const { return Side == EIJPSide::Left ? LeftScore : RightScore; }

	/** The paddle defending this side's goal. Null before BeginPlay. */
	UFUNCTION(BlueprintPure, Category = "Arena")
	AIJPPaddle* GetPaddle(EIJPSide Side) const;

	/** Plane X of the paddle lane on this side. */
	UFUNCTION(BlueprintPure, Category = "Arena")
	float GetLaneX(EIJPSide Side) const;

	/** The arena's ball. Null before BeginPlay. */
	UFUNCTION(BlueprintPure, Category = "Arena")
	AIJPBall* GetBall() const { return Ball; }

	UIJPToneSynthComponent* GetTones() const { return Tones; }
	UIJPCRTComponent* GetCRT() const { return CRT; }
	UCameraComponent* GetCamera() const { return Camera; }

	/** This screen's opponent difficulty, 0 (hopeless) .. 1 (near-perfect). */
	UFUNCTION(BlueprintPure, Category = "Arena")
	float GetOpponentSkill() const { return OpponentSkill; }

	UFUNCTION(BlueprintCallable, Category = "Arena")
	void SetOpponentSkill(float Skill) { OpponentSkill = FMath::Clamp(Skill, 0.f, 1.f); }

	/** Choose a side's paddle. Takes effect when the paddles spawn at BeginPlay. */
	void SetPaddleProfile(EIJPSide Side, UIJPPaddleProfile* Profile);

	/** The tone set in use: this arena's override, else the era's, else UIJPToneSet's defaults. */
	const UIJPToneSet& GetToneSet() const;

	/**
	 * The material that paints one palette role. From BeginPlay it's this arena's own instance,
	 * recoloured whenever the era changes, so whatever uses it follows the era for free.
	 */
	UMaterialInterface* GetPaletteMaterial(EIJPPaletteRole PaletteRole) const;

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

	/** Seconds between on/off toggles of the winner's score once a match is over. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena|Presentation", meta = (ClampMin = "0.01", Units = "s"))
	float WinnerBlinkPeriod = 0.4f;

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

	/** The player's (left) paddle. Default from DefaultGame.ini; empty uses UIJPPaddleProfile's defaults. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Arena|Paddles")
	TSoftObjectPtr<UIJPPaddleProfile> LeftPaddleProfile;

	/** The opponent's (right) paddle. Default from DefaultGame.ini; empty uses UIJPPaddleProfile's defaults. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Arena|Paddles")
	TSoftObjectPtr<UIJPPaddleProfile> RightPaddleProfile;

	/** Spawned for both sides at BeginPlay. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena|Spawning")
	TSubclassOf<AIJPPaddle> PaddleClass;

	/**
	 * How good the AI opponent is on this screen: 0 = hopeless, 1 = near-perfect. There are no
	 * difficulty settings in the game; difficulty is authored here, per arena.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena|AI", meta = (ClampMin = "0", ClampMax = "1", UIMin = "0", UIMax = "1"))
	float OpponentSkill = 0.5f;

	/** Spawned at BeginPlay, waiting at the centre until served. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena|Spawning")
	TSubclassOf<AIJPBall> BallClass;

	/** Flat unlit material for every piece, with a "Color" vector parameter the era's palette sets. From DefaultGame.ini. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Arena|Look")
	TSoftObjectPtr<UMaterialInterface> PongMaterial;

	/** The beeps for ball events on this arena only, ignoring the era. Empty = the era's tone set. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena|Audio")
	TSoftObjectPtr<UIJPToneSet> ToneSet;

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

	/** The screen behind the playfield, filling the camera's frame. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arena|Components")
	TObjectPtr<UStaticMeshComponent> Background;

	/** One cube instance per visible wall. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arena|Components")
	TObjectPtr<UInstancedStaticMeshComponent> WallVisuals;

	/** One cube instance per net dash. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arena|Components")
	TObjectPtr<UInstancedStaticMeshComponent> NetVisuals;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arena|Components")
	TObjectPtr<UIJPSevenSegmentComponent> LeftScore;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arena|Components")
	TObjectPtr<UIJPSevenSegmentComponent> RightScore;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arena|Components")
	TObjectPtr<UCameraComponent> Camera;

	/** The CRT look on Camera. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arena|Components")
	TObjectPtr<UIJPCRTComponent> CRT;

	/** The cabinet's one speaker. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arena|Components")
	TObjectPtr<UIJPToneSynthComponent> Tones;

private:
	AIJPPaddle* SpawnPaddle(EIJPSide Side);

	UFUNCTION()
	void HandleBallPaddleHit(AIJPPaddle* Paddle);

	UFUNCTION()
	void HandleBallBounce();

	UFUNCTION()
	void HandleBallGoal(EIJPSide DefendingSide);

	UFUNCTION()
	void HandleEraChanged(const UIJPEra* NewEra);

	/** Give each palette role its own material instance and put them on the arena's pieces. */
	void CreatePaletteMaterials();
	void ApplyPalette(const FIJPPalette& Palette);

	FTransform GetPlaneTransform() const;
	void AddVisualBox(UInstancedStaticMeshComponent* Target, const FVector2D& Centre, const FVector2D& Size);

	UPROPERTY(Transient)
	TObjectPtr<AIJPPaddle> LeftPaddle;

	UPROPERTY(Transient)
	TObjectPtr<AIJPPaddle> RightPaddle;

	UPROPERTY(Transient)
	TObjectPtr<AIJPBall> Ball;

	UPROPERTY(Transient)
	TObjectPtr<const UIJPToneSet> LoadedToneSet;

	/** One per EIJPPaletteRole, created at BeginPlay. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> PaletteMaterials;
};
