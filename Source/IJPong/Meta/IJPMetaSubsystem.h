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

	/** How many eras are unlocked, in order (at least 1: the Cabinet). */
	UFUNCTION(BlueprintPure, Category = "Meta")
	int32 GetErasUnlocked() const;

	/** A skill tree level is open once its era has been reached (level 0 = the first era, always open). */
	UFUNCTION(BlueprintPure, Category = "Meta")
	bool IsTreeLevelOpen(int32 Level) const { return Level < GetErasUnlocked(); }

	/** Unlock eras up to Count (never locks any). True if that unlocked a new one. */
	UFUNCTION(BlueprintCallable, Category = "Meta")
	bool UnlockErasUpTo(int32 Count);

	bool HasSeenEraCard(const FString& EraName) const;
	void MarkEraCardSeen(const FString& EraName);

	/** The Spell slot is open for every class; runs can then find spells. */
	UFUNCTION(BlueprintPure, Category = "Meta")
	bool IsSpellSlotUnlocked() const;

	UFUNCTION(BlueprintPure, Category = "Meta")
	int32 GetSpellSlotCost() const { return SpellSlotCost; }

	UFUNCTION(BlueprintPure, Category = "Meta")
	bool CanUnlockSpellSlot() const;

	/** Spend the boss tokens and open the Spell slot. */
	UFUNCTION(BlueprintCallable, Category = "Meta")
	bool UnlockSpellSlot();

	UPROPERTY(BlueprintAssignable, Category = "Meta")
	FIJPMetaChangedSignature OnMetaChanged;

protected:
	/** Save slot for the player's progress. */
	UPROPERTY(Config)
	FString SaveSlot = TEXT("IJPongMeta");

	/** Boss tokens to open the Spell slot. */
	UPROPERTY(Config)
	int32 SpellSlotCost = 2;

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
