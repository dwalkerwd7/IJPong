// It's Just Pong

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Core/IJPTypes.h"
#include "IJPGameModeBase.generated.h"

class AIJPArena;
class AIJPBall;
class AIJPPaddleAIController;
class UIJPAIProfile;
class UIJPBanterComponent;
class UIJPBossComponent;
class UIJPConversation;
class UIJPConversationPlayer;
class UIJPMatchComponent;
class UIJPMatchRules;
class UIJPRival;

/**
 * Setup shared by every IJPong mode: find the level's arena, give the player the left paddle,
 * and put an AI on the right one. Rules (serving, scoring, winning) belong to subclasses,
 * which start them from OnArenaReady().
 */
UCLASS(Abstract, Config = Game)
class IJPONG_API AIJPGameModeBase : public AGameModeBase
{
	GENERATED_BODY()

public:
	AIJPGameModeBase();

	virtual void StartPlay() override;
	virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;

	UFUNCTION(BlueprintPure, Category = "Game")
	AIJPArena* GetArena() const { return Arena; }

	UFUNCTION(BlueprintPure, Category = "Game")
	AIJPBall* GetBall() const;

	/** Runs the matches played in this mode. Subclasses decide when to start them. */
	UFUNCTION(BlueprintPure, Category = "Game")
	UIJPMatchComponent* GetMatch() const { return Match; }

	/**
	 * Who the opponent is: their class goes on the opponent's paddle and their style on its AI, now.
	 * Null = the arena's own class for that side and the default AI profile.
	 */
	UFUNCTION(BlueprintCallable, Category = "Game")
	void SetRival(const UIJPRival* InRival);

	UFUNCTION(BlueprintPure, Category = "Game")
	const UIJPRival* GetRival() const { return Rival; }

	/**
	 * Start a new match on the arena. If the rival has pre-match conversations, one plays first and
	 * the first serve waits for it. Null Rules uses UIJPMatchRules' defaults.
	 */
	UFUNCTION(BlueprintCallable, Category = "Game")
	void BeginMatch(const UIJPMatchRules* Rules);

	/** Play a conversation in the paddles' chat bubbles now, replacing any in progress. Doesn't pause play. */
	UFUNCTION(BlueprintCallable, Category = "Game")
	void PlayConversation(const UIJPConversation* Conversation);

	UFUNCTION(BlueprintPure, Category = "Game")
	UIJPConversationPlayer* GetConversations() const { return Conversations; }

	UIJPBanterComponent* GetBanter() const { return Banter; }

	UIJPBossComponent* GetBoss() const { return Boss; }

	/** The opponent's voice when no rival is set (a multiple of the tone set's Talk pitch). */
	static constexpr float DefaultOpponentVoice = 0.8f;

	/** Set how well the opponent plays (0..1) on the arena, and on its AI now. */
	UFUNCTION(BlueprintCallable, Category = "Game")
	void SetOpponentSkill(float Skill);

	/**
	 * Menu-style input (a map, later menus). Step = -1 left / +1 right; Confirm = the class-skill
	 * button. Return true to consume it; false lets it through to the paddle as usual.
	 */
	virtual bool HandleUIStep(int32 Direction) { return false; }
	virtual bool HandleUIConfirm() { return false; }

	/** The side the local player plays. */
	static constexpr EIJPSide PlayerSide = EIJPSide::Left;

protected:
	/** Called at the end of StartPlay, once the paddles and ball exist and have their controllers. */
	virtual void OnArenaReady() {}

	/** Give the player PlayerSide's paddle, if it's free. */
	void PossessPlayerPaddle(APlayerController* PlayerController);

	/** Spawn an AI to play Side's paddle, if it's free. */
	AIJPPaddleAIController* SpawnAIPaddle(EIJPSide Side);

	/** Controller class for AI paddles. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Game|AI")
	TSubclassOf<AIJPPaddleAIController> AIControllerClass;

	/** How the AI plays. Set in DefaultGame.ini; empty uses UIJPAIProfile's defaults. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Game|AI")
	TSoftObjectPtr<UIJPAIProfile> AIProfile;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Game|Components")
	TObjectPtr<UIJPMatchComponent> Match;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Game|Components")
	TObjectPtr<UIJPConversationPlayer> Conversations;

	/** The current rival's mid-rally reactions. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Game|Components")
	TObjectPtr<UIJPBanterComponent> Banter;

	/** The current rival's boss phases (if it's a boss). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Game|Components")
	TObjectPtr<UIJPBossComponent> Boss;

private:
	UFUNCTION()
	void HandleMatchEnded(EIJPSide Winner);

	UFUNCTION()
	void HandleConversationFinished(const UIJPConversation* Conversation);

	/** The AI profile for Side's AI: the rival's on the opponent's side, else the configured one. */
	const UIJPAIProfile* GetAIProfileFor(EIJPSide Side) const;

	UPROPERTY(Transient)
	TObjectPtr<AIJPArena> Arena;

	UPROPERTY(Transient)
	TObjectPtr<const UIJPRival> Rival;
};
