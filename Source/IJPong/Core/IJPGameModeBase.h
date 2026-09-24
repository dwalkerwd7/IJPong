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

private:
	UPROPERTY(Transient)
	TObjectPtr<AIJPArena> Arena;
};
