// It's Just Pong

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Run/IJPRunMap.h"
#include "IJPRunSubsystem.generated.h"

class UIJPAbility;
class UIJPActConfig;
class UIJPBallType;
class UIJPReward;

UENUM(BlueprintType)
enum class EIJPRunState : uint8
{
	/** No run started. */
	None,
	Running,
	/** Health ran out. */
	Lost,
	/** The act's boss was beaten. */
	Won
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FIJPRunChangedSignature);

/** What the player has gathered this run, applied to every fight (see AIJPRunGameMode). */
USTRUCT(BlueprintType)
struct FIJPRunLoadout
{
	GENERATED_BODY()

	/** Added to the paddle's length, as a fraction (0.2 = +20%). */
	UPROPERTY(BlueprintReadOnly, Category = "Loadout")
	float PaddleLengthBonus = 0.f;

	/** Added to the paddle's top speed, as a fraction. */
	UPROPERTY(BlueprintReadOnly, Category = "Loadout")
	float PaddleSpeedBonus = 0.f;

	/** Cut from the class skill's cooldown, as a fraction. */
	UPROPERTY(BlueprintReadOnly, Category = "Loadout")
	float ClassSkillCooldownCut = 0.f;

	/** In the run-ability slot. Null = empty. */
	UPROPERTY(BlueprintReadOnly, Category = "Loadout")
	TObjectPtr<const UIJPAbility> RunAbility;

	/** Launched on every serve, on top of the match's own. */
	UPROPERTY(BlueprintReadOnly, Category = "Loadout")
	TArray<TObjectPtr<const UIJPBallType>> ExtraServedBalls;

	/** The one item carried (Item slot), or null. A new one replaces it; using it clears it. */
	UPROPERTY(BlueprintReadOnly, Category = "Loadout")
	TObjectPtr<const UIJPAbility> Item;
};

/**
 * The run in progress: its act and map, where the player is on it, and the run-wide health and
 * coins. Lives on the game instance, so it outlasts levels (and is where saving will hook in).
 * It knows the rules of moving along the map, not how nodes are played: the run game mode plays
 * a node and reports back with CompleteNode.
 */
UCLASS()
class IJPONG_API UIJPRunSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	static UIJPRunSubsystem* Get(const UObject* WorldContext);

	/** Begin a fresh run of Act from its top row. Seed makes the map reproducible. */
	UFUNCTION(BlueprintCallable, Category = "Run")
	void StartRun(const UIJPActConfig* Act, int32 Seed, float StartingHealth);

	UFUNCTION(BlueprintPure, Category = "Run")
	EIJPRunState GetState() const { return State; }

	const FIJPRunMap& GetMap() const { return Map; }
	const UIJPActConfig* GetAct() const { return Act; }

	/** The node the player last entered, or INDEX_NONE before the first. */
	UFUNCTION(BlueprintPure, Category = "Run")
	int32 GetCurrentNode() const { return CurrentNode; }

	/** Being played right now (a fight in progress). */
	UFUNCTION(BlueprintPure, Category = "Run")
	bool IsInNode() const { return bInNode; }

	/** Nodes the player may enter next, left to right. Empty while in a node, while a reward is on offer, or once the run is over. */
	UFUNCTION(BlueprintPure, Category = "Run")
	TArray<int32> GetReachableNodes() const;

	UFUNCTION(BlueprintPure, Category = "Run")
	bool IsVisited(int32 Node) const { return Visited.IsValidIndex(Node) && Visited[Node]; }

	/**
	 * Move onto a reachable node. A Rest node heals at once and is done; a fight node stays
	 * "in node" until CompleteNode. False if the node isn't reachable now.
	 */
	UFUNCTION(BlueprintCallable, Category = "Run")
	bool EnterNode(int32 Node);

	/**
	 * The fight at the current node ended. A win pays its coins and, for Match and Elite, puts a pick
	 * of rewards on offer (see TakeReward). Winning the boss wins the run.
	 */
	UFUNCTION(BlueprintCallable, Category = "Run")
	void CompleteNode(bool bWon);

	/** Lose health (damage the player took in a match). At 0 the run is lost. */
	UFUNCTION(BlueprintCallable, Category = "Run")
	void LoseHealth(float Amount);

	UFUNCTION(BlueprintPure, Category = "Run")
	float GetHealth() const { return Health; }

	UFUNCTION(BlueprintPure, Category = "Run")
	float GetMaxHealth() const { return MaxHealth; }

	UFUNCTION(BlueprintPure, Category = "Run")
	int32 GetCoins() const { return Coins; }

	// --- Rewards and loadout ---

	/** Rewards waiting to be picked (the map is on hold until one is taken or skipped). */
	UFUNCTION(BlueprintPure, Category = "Run")
	bool HasOffer() const { return !Offer.IsEmpty(); }

	const TArray<TObjectPtr<const UIJPReward>>& GetOffer() const { return Offer; }

	/** Take the offered reward at Index, or skip for the act's SkipCoins with INDEX_NONE. */
	UFUNCTION(BlueprintCallable, Category = "Run")
	void TakeReward(int32 Index);

	const FIJPRunLoadout& GetLoadout() const { return Loadout; }

	/** For rewards granting themselves. */
	FIJPRunLoadout& EditLoadout() { return Loadout; }

	/** Heal up to the maximum (e.g. an item used mid-fight). */
	UFUNCTION(BlueprintCallable, Category = "Run")
	void RestoreHealth(float Amount);

	/** Raise the maximum and heal the same amount. */
	void AddMaxHealth(float Amount);

	// --- Meta currencies earned by this run (already added to UIJPMetaSubsystem) ---

	UFUNCTION(BlueprintPure, Category = "Run")
	int32 GetEarnedSkillPoints() const { return EarnedSkillPoints; }

	UFUNCTION(BlueprintPure, Category = "Run")
	int32 GetEarnedBossTokens() const { return EarnedBossTokens; }

	/** How many rows down the run got: the deepest node entered, counting from 1. */
	UFUNCTION(BlueprintPure, Category = "Run")
	int32 GetDepthReached() const;

	/** Anything about the run changed (moved, health, coins, state). */
	UPROPERTY(BlueprintAssignable, Category = "Run")
	FIJPRunChangedSignature OnRunChanged;

private:
	void Heal(float Amount);
	void RollOffer(const TArray<TObjectPtr<UIJPReward>>& Pool);
	/** The run just ended (won or lost): pay its skill points into the meta currencies. */
	void PayOut();

	UPROPERTY(Transient)
	TObjectPtr<const UIJPActConfig> Act;

	UPROPERTY(Transient)
	TArray<TObjectPtr<const UIJPReward>> Offer;

	UPROPERTY(Transient)
	FIJPRunLoadout Loadout;

	FIJPRunMap Map;
	FRandomStream Random;
	TArray<bool> Visited;
	EIJPRunState State = EIJPRunState::None;
	int32 CurrentNode = INDEX_NONE;
	bool bInNode = false;
	float Health = 0.f;
	float MaxHealth = 0.f;
	int32 Coins = 0;
	int32 EarnedSkillPoints = 0;
	int32 EarnedBossTokens = 0;
};
