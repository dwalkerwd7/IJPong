// It's Just Pong

#pragma once

#include "CoreMinimal.h"
#include "Abilities/IJPAbility.h"
#include "Core/IJPGameModeBase.h"
#include "Run/IJPRunSubsystem.h"
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
	/** Picking a reward after a win (or skipping it for coins). */
	Reward,
	/** At a Shop node: buying with coins, then LEAVE. */
	Shop,
	/** Between two acts in different eras: the tube switches off, the title card, then the new era. */
	EraChange,
	/** The run is over (won or lost); confirm opens the skill tree (or starts a new run if the class has none). */
	Ended,
	/** Between runs: spending meta currencies on the class's skill tree, then START RUN. */
	Tree
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
	void StartNewRun(const UIJPActConfig* Act = nullptr, int32 Seed = -1, float InStartingHealth = 0.f);

	/** Start a run climbing through Stages (each an act in an era); UnlocksErasTo as UIJPRunSubsystem::StartRun. */
	void StartClimb(const TArray<FIJPRunStage>& Stages, int32 Seed = -1, float InStartingHealth = 0.f, int32 UnlocksErasTo = 0);

	/** The climb a new run takes: every unlocked era in order (beaten ones briefly, the newest in full). */
	TArray<FIJPRunStage> BuildClimb(int32& OutUnlocksErasTo) const;

	/** How long the era change (switch-off and title card) takes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Run")
	float EraChangeTime = 3.f;

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
	float StartingHealth = 10.f;

private:
	void EnterSelectedNode();
	/** Put what the run has gathered on the player's paddle and serves. */
	void ApplyLoadout();
	void ShowRewards();
	/** The shop's shelf as cards (with prices) plus LEAVE; SelectedCard keeps the pick after a purchase. */
	void ShowShop(int32 SelectedCard = 0);
	const class UIJPSkillTree* GetPlayerTree() const;
	void FinishNode();
	void BeginEraChange(const UIJPEra* NewEra);
	void FinishEraChange();
	void EndRun();
	void ShowMap();
	void ShowArena();
	void SetViewTarget(AActor* Target) const;

	/** Damage the player takes in a match comes off the run's health. */
	UFUNCTION()
	void HandleHealthChanged(EIJPSide Side, float Health, float Damage);

	UFUNCTION()
	void HandleRunMatchEnded(EIJPSide Winner);

	/** The player used their item: it's gone from the run too. */
	UFUNCTION()
	void HandlePlayerAbility(EIJPAbilitySlot Slot, const UIJPAbility* Ability);

	UPROPERTY(Transient)
	TObjectPtr<AIJPRunMapView> MapView;

	/** What the current run was started with, so "new run" replays the same act. */
	UPROPERTY(Transient)
	TObjectPtr<const UIJPActConfig> RunAct;

	float RunStartingHealth = 0.f;

	FTimerHandle AfterMatchTimer;
	FTimerHandle EraChangeTimer;

	/** The era the change is heading to. */
	UPROPERTY(Transient)
	TObjectPtr<const UIJPEra> PendingEra;
	EIJPRunPhase Phase = EIJPRunPhase::Map;
	bool bLastMatchWon = false;
};
