// It's Just Pong

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "IJPMetaSubsystem.generated.h"

class UIJPMetaSave;
class UIJPSkillTree;
struct FIJPTreeBonuses;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FIJPMetaChangedSignature);

/**
 * The player's progress between runs: the meta currencies (shared by every class), and later
 * the skill trees and unlocked eras. Loaded when the game starts and saved to disk every time it
 * changes, so nothing earned is lost if the game closes.
 * Automation tests use their own save slot, so they never touch the player's progress.
 */
UCLASS(Config = Game)
class IJPONG_API UIJPMetaSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	static UIJPMetaSubsystem* Get(const UObject* WorldContext);

	UFUNCTION(BlueprintPure, Category = "Meta")
	int32 GetSkillPoints() const;

	UFUNCTION(BlueprintPure, Category = "Meta")
	int32 GetBossTokens() const;

	/** Add to the currencies (negative spends) and save. */
	UFUNCTION(BlueprintCallable, Category = "Meta")
	void AddCurrency(int32 SkillPoints, int32 BossTokens);

	// --- Skill trees ---

	UFUNCTION(BlueprintPure, Category = "Meta")
	bool IsOwned(const UIJPSkillTree* Tree, int32 Node) const;

	/** Not owned yet, its parent is (or it hangs from the root), and it's affordable. */
	UFUNCTION(BlueprintPure, Category = "Meta")
	bool CanBuy(const UIJPSkillTree* Tree, int32 Node) const;

	/** Pay for and own a node (and save). False if it can't be bought now. */
	UFUNCTION(BlueprintCallable, Category = "Meta")
	bool Buy(const UIJPSkillTree* Tree, int32 Node);

	/** What the owned nodes of Tree add up to. */
	FIJPTreeBonuses GetBonuses(const UIJPSkillTree* Tree) const;

	/** Wipe all progress (and the save). */
	UFUNCTION(BlueprintCallable, Category = "Meta")
	void ResetProgress();

	UPROPERTY(BlueprintAssignable, Category = "Meta")
	FIJPMetaChangedSignature OnMetaChanged;

protected:
	/** Save slot for the player's progress. */
	UPROPERTY(Config)
	FString SaveSlot = TEXT("IJPongMeta");

	/** Save slot used while automation tests run. */
	UPROPERTY(Config)
	FString TestSaveSlot = TEXT("IJPongMeta_Tests");

private:
	FString GetSlot() const;
	static FString NodeKey(const UIJPSkillTree& Tree, int32 Node);
	void Save() const;

	UPROPERTY(Transient)
	TObjectPtr<UIJPMetaSave> SaveData;
};
