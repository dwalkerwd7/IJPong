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
struct FIJPEncounter;

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
	/** At an Event node: its text, and its options as cards. */
	Event,
	/** An event's option played out: what happened, then back to the map. */
	EventResult,
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
	AIJPRunGameMode();

	// --- Debug cheats (keys in AIJPRunPlayerController; never bound in shipping builds) ---

	/** End the match being played now: the player wins (the rival's health to 0) or loses (theirs to 0). */
	void CheatEndMatch(bool bWin);
	/** The run's health to full (and the player's in a match being played). */
	void CheatHeal();
	void CheatCoins(int32 Amount);
	/** Meta currencies, kept between runs. */
	void CheatMeta(int32 SkillPoints, int32 BossTokens);
	/** One more era unlocked: the next run climbs into it. */
	void CheatUnlockNextEra();

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
	virtual bool HandleUIStepVertical(int32 Direction) override;
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
	/** Play Encounter's match. PlayerHealth > 0: the player's health in it (losses still come off the run); 0 = the run's. */
	void BeginFight(const FIJPEncounter& Encounter, float PlayerHealth = 0.f);
	/** Put what the run has gathered on the player's paddle and serves. */
	void ApplyLoadout();
	void ShowRewards();
	/** The shop's shelf as cards (with prices) plus LEAVE; SelectedCard keeps the pick after a purchase. */
	void ShowShop(int32 SelectedCard = 0);
	void ShowEvent();
	/** Redraw whatever screen is up (after a cheat changed what it shows). */
	void RefreshScreen();
	/** A screen of cards to pick from (map, rewards, shop, event, tree): the arrows step the pick. */
	bool IsPickingPhase() const
	{
		return Phase == EIJPRunPhase::Map || Phase == EIJPRunPhase::Reward || Phase == EIJPRunPhase::Shop
			|| Phase == EIJPRunPhase::Event || Phase == EIJPRunPhase::EventResult || Phase == EIJPRunPhase::Tree;
	}
	void ShowEventResult();
	/** "HP 7/10    COINS 25", for headings. */
	FString GetRunStatus() const;
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

	/** A bonus target broke: coins if the player's return broke it. */
	UFUNCTION()
	void HandleBonusTarget(EIJPSide Side, int32 Coins);

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
