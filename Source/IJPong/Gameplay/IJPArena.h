// It's Just Pong

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/IJPTypes.h"
#include "Era/IJPEra.h"
#include "Gameplay/IJPSpellStrike.h"
#include "IJPArena.generated.h"

class UBoxComponent;
class UCameraComponent;
class UInstancedStaticMeshComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UStaticMeshComponent;
class AIJPBall;
class UIJPBallType;
class AIJPPaddle;
class UIJPPaddleClass;
class UIJPCRTComponent;
class UIJPGoalComponent;
class UIJPSevenSegmentComponent;
class UIJPChargePipsComponent;
class AIJPSpellStrike;
class UIJPToneSet;
class UIJPToneSynthComponent;
class UIJPEra;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FIJPBarrierHitSignature, EIJPSide, Side, AIJPBall*, Ball);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FIJPArenaBallGoalSignature, AIJPBall*, Ball, EIJPSide, DefendingSide);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FIJPArenaBallReturnedSignature, AIJPBall*, Ball, AIJPPaddle*, Paddle);

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

	/** Raise or drop Side's barrier: a ball-blocking line just in front of that side's goal. */
	UFUNCTION(BlueprintCallable, Category = "Arena")
	void SetBarrierUp(EIJPSide Side, bool bUp);

	UFUNCTION(BlueprintPure, Category = "Arena")
	bool IsBarrierUp(EIJPSide Side) const;

	/** The ball-blocking line inside Side's goal (blocks only while up). */
	UPrimitiveComponent* GetBarrier(EIJPSide Side) const;

	/** Called by a ball that just bounced off Side's barrier. */
	void NotifyBarrierHit(EIJPSide Side, AIJPBall* Ball) { OnBarrierHit.Broadcast(Side, Ball); }

	/** A ball bounced off Side's barrier (items like Shield drop it after one block). */
	UPROPERTY(BlueprintAssignable, Category = "Arena")
	FIJPBarrierHitSignature OnBarrierHit;

	UIJPSevenSegmentComponent* GetScoreDisplay(EIJPSide Side) const { return Side == EIJPSide::Left ? LeftScore : RightScore; }

	/** Side's spell charge, as pips under its health (Total 0 hides them). */
	void SetChargePips(EIJPSide Side, int32 Filled, int32 Total);

	UIJPChargePipsComponent* GetChargePips(EIJPSide Side) const { return Side == EIJPSide::Left ? LeftPips : RightPips; }

	/** Spells on their way in (so the AI can steer clear). */
	void RegisterStrike(AIJPSpellStrike* Strike) { Strikes.AddUnique(Strike); }
	void UnregisterStrike(AIJPSpellStrike* Strike) { Strikes.Remove(Strike); }
	const TArray<TWeakObjectPtr<AIJPSpellStrike>>& GetStrikes() const { return Strikes; }

	/** The paddle defending this side's goal. Null before BeginPlay. */
	UFUNCTION(BlueprintPure, Category = "Arena")
	AIJPPaddle* GetPaddle(EIJPSide Side) const;

	/** Plane X of the paddle lane on this side. */
	UFUNCTION(BlueprintPure, Category = "Arena")
	float GetLaneX(EIJPSide Side) const;

	/** The main ball: the first one, which always exists and is the one blinking before a serve. Null before BeginPlay. */
	UFUNCTION(BlueprintPure, Category = "Arena")
	AIJPBall* GetBall() const { return Balls.IsEmpty() ? nullptr : Balls[0].Get(); }

	/** Every ball the arena has, in play or waiting out of play. */
	const TArray<TObjectPtr<AIJPBall>>& GetBalls() const { return Balls; }

	/**
	 * A ball of Type, out of play at the centre, ready to Serve(). Reuses a waiting ball when there
	 * is one, else spawns another. Null Type = the arena's default ball type.
	 */
	UFUNCTION(BlueprintCallable, Category = "Arena")
	AIJPBall* AddBall(const UIJPBallType* Type);

	UFUNCTION(BlueprintPure, Category = "Arena")
	int32 GetNumBallsInPlay() const;

	/** Take every ball out of play. */
	UFUNCTION(BlueprintCallable, Category = "Arena")
	void ResetBalls();

	/** The ball type used when none is given: DefaultBallType, or UIJPBallType's defaults. */
	const UIJPBallType* GetDefaultBallType() const;

	/** The colour a ball of Type is drawn in the current era. */
	FLinearColor GetBallColour(const UIJPBallType* Type) const;

	/** The era palette on screen now. */
	const FIJPPalette& GetPalette() const { return CurrentPalette; }

	/** Material for sprites (M_PongSprite): a texture times a colour, cut out by its alpha, with 9-slice stretching. */
	UMaterialInterface* GetSpriteMaterial() const { return SpriteMaterial.LoadSynchronous(); }

	/** The current era shows sprites (paddles, balls). */
	bool ShowsSprites() const;

	/** Material for chat-bubble panels (M_PongBubble): a box with a tail, drawn from parameters. */
	UMaterialInterface* GetBubbleMaterial() const { return BubbleMaterial.LoadSynchronous(); }

	/** Material for text in the arena (chat bubbles): unlit, coloured by the text's own colour. */
	UMaterialInterface* GetTextMaterial() const { return TextMaterial.LoadSynchronous(); }

	/** The plain material every piece is made from (before palette colours). */
	UMaterialInterface* GetBaseMaterial() const { return PongMaterial.Get(); }

	/** Any ball went into a goal. DefendingSide concedes; Ball says what it was worth. */
	UPROPERTY(BlueprintAssignable, Category = "Arena")
	FIJPArenaBallGoalSignature OnBallGoal;

	/** A paddle returned a ball (its rally count already includes this hit). */
	UPROPERTY(BlueprintAssignable, Category = "Arena")
	FIJPArenaBallReturnedSignature OnBallReturned;

	UIJPToneSynthComponent* GetTones() const { return Tones; }
	UIJPCRTComponent* GetCRT() const { return CRT; }
	UCameraComponent* GetCamera() const { return Camera; }

	/** This screen's opponent difficulty, 0 (hopeless) .. 1 (near-perfect). */
	UFUNCTION(BlueprintPure, Category = "Arena")
	float GetOpponentSkill() const { return OpponentSkill; }

	UFUNCTION(BlueprintCallable, Category = "Arena")
	void SetOpponentSkill(float Skill) { OpponentSkill = FMath::Clamp(Skill, 0.f, 1.f); }

	/** Choose a side's paddle class. Before BeginPlay it's used at spawn; after, the paddle switches now. */
	UFUNCTION(BlueprintCallable, Category = "Arena")
	void SetPaddleClass(EIJPSide Side, const UIJPPaddleClass* SideClass);

	/** The class this arena was set up with for Side (config, level, or SetPaddleClass before BeginPlay). */
	const UIJPPaddleClass* GetConfiguredPaddleClass(EIJPSide Side) const;

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

	/** Barrier lines (see SetBarrierUp): distance of their centre in from the goal line, and thickness. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena|Layout")
	float BarrierInset = 12.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena|Layout")
	float BarrierThickness = 6.f;

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

	/** The player's (left) paddle class. Default from DefaultGame.ini; empty = default stats, no skill. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Arena|Paddles")
	TSoftObjectPtr<UIJPPaddleClass> LeftPaddleClass;

	/** The opponent's (right) paddle class. Default from DefaultGame.ini; a rival can replace it. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Arena|Paddles")
	TSoftObjectPtr<UIJPPaddleClass> RightPaddleClass;

	/** Spawned for both sides at BeginPlay. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena|Spawning")
	TSubclassOf<AIJPPaddle> PaddleClass;

	/**
	 * How good the AI opponent is on this screen: 0 = hopeless, 1 = near-perfect. There are no
	 * difficulty settings in the game; difficulty is authored here, per arena.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena|AI", meta = (ClampMin = "0", ClampMax = "1", UIMin = "0", UIMax = "1"))
	float OpponentSkill = 0.5f;

	/** Class of every ball. The main ball spawns at BeginPlay, waiting at the centre until served. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena|Spawning")
	TSubclassOf<AIJPBall> BallClass;

	/** Type of the main ball, and of any ball added without a type. From DefaultGame.ini; empty = UIJPBallType's defaults. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Arena|Spawning")
	TSoftObjectPtr<UIJPBallType> DefaultBallType;

	/** Flat unlit material for every piece, with a "Color" vector parameter the era's palette sets. From DefaultGame.ini. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Arena|Look")
	TSoftObjectPtr<UMaterialInterface> PongMaterial;

	/** Sprite material (M_PongSprite via DefaultGame.ini). */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Arena|Look")
	TSoftObjectPtr<UMaterialInterface> SpriteMaterial;

	/** Chat-bubble panel material (M_PongBubble via DefaultGame.ini). */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Arena|Look")
	TSoftObjectPtr<UMaterialInterface> BubbleMaterial;

	/** Unlit text material (M_PongText via DefaultGame.ini). */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Arena|Look")
	TSoftObjectPtr<UMaterialInterface> TextMaterial;

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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arena|Components")
	TObjectPtr<UBoxComponent> LeftBarrier;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arena|Components")
	TObjectPtr<UBoxComponent> RightBarrier;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arena|Components")
	TObjectPtr<UStaticMeshComponent> LeftBarrierVisual;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arena|Components")
	TObjectPtr<UStaticMeshComponent> RightBarrierVisual;

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

	/** Spell charge under each side's health. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arena|Components")
	TObjectPtr<UIJPChargePipsComponent> LeftPips;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arena|Components")
	TObjectPtr<UIJPChargePipsComponent> RightPips;

	TArray<TWeakObjectPtr<AIJPSpellStrike>> Strikes;

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
	void HandleBallPaddleHit(AIJPBall* HitBall, AIJPPaddle* Paddle);

	UFUNCTION()
	void HandleBallBounce();

	UFUNCTION()
	void HandleBallGoal(AIJPBall* ScoringBall, EIJPSide DefendingSide);

	AIJPBall* SpawnBall(const UIJPBallType* Type);

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

	/** Balls[0] is the main ball. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<AIJPBall>> Balls;

	/** The palette on screen now, for colouring balls as they appear. */
	FIJPPalette CurrentPalette;

	UPROPERTY(Transient)
	TObjectPtr<const UIJPToneSet> LoadedToneSet;

	/** One per EIJPPaletteRole, created at BeginPlay. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> PaletteMaterials;
};
