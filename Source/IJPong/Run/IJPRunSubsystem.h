// It's Just Pong

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Run/IJPRunMap.h"
#include "IJPRunSubsystem.generated.h"

class UIJPActConfig;

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
	void StartRun(const UIJPActConfig* Act, int32 Seed, int32 StartingHealth);

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

	/** Nodes the player may enter next, left to right. Empty while in a node or once the run is over. */
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

	/** The fight at the current node ended. A win pays its coins; winning the boss wins the run. */
	UFUNCTION(BlueprintCallable, Category = "Run")
	void CompleteNode(bool bWon);

	/** Lose health (a goal against the player). At 0 the run is lost. */
	UFUNCTION(BlueprintCallable, Category = "Run")
	void LoseHealth(int32 Amount = 1);

	UFUNCTION(BlueprintPure, Category = "Run")
	int32 GetHealth() const { return Health; }

	UFUNCTION(BlueprintPure, Category = "Run")
	int32 GetMaxHealth() const { return MaxHealth; }

	UFUNCTION(BlueprintPure, Category = "Run")
	int32 GetCoins() const { return Coins; }

	/** Anything about the run changed (moved, health, coins, state). */
	UPROPERTY(BlueprintAssignable, Category = "Run")
	FIJPRunChangedSignature OnRunChanged;

private:
	void Heal(int32 Amount);

	UPROPERTY(Transient)
	TObjectPtr<const UIJPActConfig> Act;

	FIJPRunMap Map;
	TArray<bool> Visited;
	EIJPRunState State = EIJPRunState::None;
	int32 CurrentNode = INDEX_NONE;
	bool bInNode = false;
	int32 Health = 0;
	int32 MaxHealth = 0;
	int32 Coins = 0;
};
