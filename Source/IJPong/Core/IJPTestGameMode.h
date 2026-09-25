// It's Just Pong

#pragma once

#include "CoreMinimal.h"
#include "Core/IJPGameModeBase.h"
#include "IJPTestGameMode.generated.h"

class UIJPAbility;
class UIJPBallType;
class UIJPMatchRules;
class UIJPPaddleClass;
class UIJPRival;

/**
 * Match after match for trying out a level, with the rules from config (MatchRules).
 * Also has test tools (bound to debug keys by AIJPTestPlayerController): start a new match,
 * serve now, hand the player's paddle to an AI to watch the level play itself, nudge the
 * opponent's skill up and down, step through the eras, and throw extra balls into the rally.
 */
UCLASS()
class IJPONG_API AIJPTestGameMode : public AIJPGameModeBase
{
	GENERATED_BODY()

public:
	AIJPTestGameMode();

	/** Abandon the current match and start a new one. Null Rules uses the configured MatchRules. */
	UFUNCTION(BlueprintCallable, Category = "Test")
	void RestartMatch(const UIJPMatchRules* Rules = nullptr);

	/** Serve immediately toward a random side, abandoning any rally in progress. Once a match is over, starts a new one. */
	UFUNCTION(BlueprintCallable, Category = "Test")
	void ServeNow();

	/** Hand the player's paddle to an AI (true) or give it back to the player (false). */
	UFUNCTION(BlueprintCallable, Category = "Test")
	void SetPlayerSideAI(bool bEnable);

	UFUNCTION(BlueprintPure, Category = "Test")
	bool IsPlayerSideAI() const { return PlayerSideAI != nullptr; }

	/** Nudge the arena's opponent skill (clamped 0..1) and apply it live to every AI paddle. Shows the new value on screen. */
	UFUNCTION(BlueprintCallable, Category = "Test")
	void AdjustOpponentSkill(float Delta);

	/** Step Direction eras through the configured list, wrapping at both ends. Shows the new era on screen. */
	UFUNCTION(BlueprintCallable, Category = "Test")
	void CycleEra(int32 Direction);

	/** Step the player's paddle through PlayerClasses (wrapping). */
	UFUNCTION(BlueprintCallable, Category = "Test")
	void CyclePlayerClass(int32 Direction);

	/** Step the opponent through Rivals, with "no rival" (the arena's own class) between the last and the first. */
	UFUNCTION(BlueprintCallable, Category = "Test")
	void CycleRival(int32 Direction);

	/** Launch one extra ball of a random ExtraBallTypes type into the current rally. */
	UFUNCTION(BlueprintCallable, Category = "Test")
	void AddRandomBall();

	/** The debug overlay's text: the test tools' current state and the key legend. */
	void GetDebugLines(TArray<FString>& OutLines) const;

protected:
	virtual void OnArenaReady() override;

	/** The matches played here. Set in DefaultGame.ini; empty uses UIJPMatchRules' defaults. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Test|Match")
	TSoftObjectPtr<UIJPMatchRules> MatchRules;

	/** Equipped in the player paddle's run-ability slot (until runs hand them out). Set in DefaultGame.ini. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Test|Abilities")
	TSoftObjectPtr<UIJPAbility> PlayerRunAbility;

	/** Classes the class key steps through. Set in DefaultGame.ini. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Test|Classes")
	TArray<TSoftObjectPtr<UIJPPaddleClass>> PlayerClasses;

	/** Rivals the rival key steps through. Set in DefaultGame.ini. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Test|Classes")
	TArray<TSoftObjectPtr<UIJPRival>> Rivals;

	/** Types the add-ball key picks from. Set in DefaultGame.ini; empty = the arena's default type. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Test|Balls")
	TArray<TSoftObjectPtr<UIJPBallType>> ExtraBallTypes;

private:
	void ShowMessage(int32 Key, const FString& Message) const;

	UPROPERTY(Transient)
	TObjectPtr<AIJPPaddleAIController> PlayerSideAI;

	/** Position in Rivals; -1 = no rival. */
	int32 RivalIndex = -1;
};
