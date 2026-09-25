// It's Just Pong

#pragma once

#include "CoreMinimal.h"
#include "Core/IJPGameModeBase.h"
#include "IJPRunGameMode.generated.h"

class AIJPBall;
class AIJPRunMapView;
class UIJPActConfig;

UENUM(BlueprintType)
enum class EIJPRunPhase : uint8
{
	/** Picking the next node on the map. */
	Map,
	/** A fight node's match is being played. */
	Playing,
	/** The match just ended; the result shows for a moment before the map comes back. */
	AfterMatch,
	/** The run is over (won or lost); confirm starts a new one. */
	Ended
};

/**
 * Plays a run: the map (AIJPRunMapView) to pick a node, the arena to play it, and back. The run's
 * state lives in UIJPRunSubsystem; this decides what each node means. Fight nodes are matches
 * against the act's rivals at its difficulty; every goal against the player costs run health, and
 * a run ends at 0 health or when the boss falls. Rest heals on the spot.
 */
UCLASS(Config = Game)
class IJPONG_API AIJPRunGameMode : public AIJPGameModeBase
{
	GENERATED_BODY()

public:
	/**
	 * Throw away any run and start a fresh one on the map.
	 * @param Act             Null = FirstAct from config.
	 * @param Seed            INDEX_NONE = random.
	 * @param InStartingHealth 0 = StartingHealth from config.
	 */
	UFUNCTION(BlueprintCallable, Category = "Run")
	void StartNewRun(const UIJPActConfig* Act = nullptr, int32 Seed = -1, int32 InStartingHealth = 0);

	UFUNCTION(BlueprintPure, Category = "Run")
	EIJPRunPhase GetPhase() const { return Phase; }

	AIJPRunMapView* GetMapView() const { return MapView; }

	virtual bool HandleUIStep(int32 Direction) override;
	virtual bool HandleUIConfirm() override;

	/** Seconds the result of a match stays up (winner blink, rival's last word) before the map returns. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Run")
	float PostMatchDelay = 3.f;

protected:
	virtual void OnArenaReady() override;

	/** The act a run starts with. Set in DefaultGame.ini. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Run")
	TSoftObjectPtr<UIJPActConfig> FirstAct;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Run", meta = (ClampMin = "1"))
	int32 StartingHealth = 10;

private:
	void EnterSelectedNode();
	void FinishNode();
	void EndRun();
	void ShowMap();
	void ShowArena();
	void SetViewTarget(AActor* Target) const;

	UFUNCTION()
	void HandleBallGoal(AIJPBall* ScoringBall, EIJPSide DefendingSide);

	UFUNCTION()
	void HandleRunMatchEnded(EIJPSide Winner);

	UPROPERTY(Transient)
	TObjectPtr<AIJPRunMapView> MapView;

	/** What the current run was started with, so "new run" replays the same act. */
	UPROPERTY(Transient)
	TObjectPtr<const UIJPActConfig> RunAct;

	int32 RunStartingHealth = 0;

	FTimerHandle AfterMatchTimer;
	EIJPRunPhase Phase = EIJPRunPhase::Map;
	bool bLastMatchWon = false;
};
