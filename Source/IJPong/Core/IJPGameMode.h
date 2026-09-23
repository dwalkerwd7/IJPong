// It's Just Pong

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "IJPGameMode.generated.h"

class AIJPArena;

/**
 * For now: finds the level's arena and gives the player its left paddle.
 * This is the class that grows into the match state machine.
 */
UCLASS()
class IJPONG_API AIJPGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AIJPGameMode();

	virtual void StartPlay() override;
	virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;

	UFUNCTION(BlueprintPure, Category = "Game")
	AIJPArena* GetArena() const { return Arena; }

private:
	void PossessPlayerPaddle(APlayerController* PlayerController);

	UPROPERTY(Transient)
	TObjectPtr<AIJPArena> Arena;
};
