// It's Just Pong

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Core/IJPTypes.h"
#include "IJPGameMode.generated.h"

class AIJPArena;
class AIJPPaddleAIController;
class UIJPAIProfile;

/**
 * For now: finds the level's arena, gives the player its left paddle and an AI the right one,
 * and runs an endless serve -> goal -> score -> serve loop.
 * This is the class that grows into the match state machine.
 */
UCLASS(Config = Game)
class IJPONG_API AIJPGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AIJPGameMode();

	virtual void StartPlay() override;
	virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;

	UFUNCTION(BlueprintPure, Category = "Game")
	AIJPArena* GetArena() const { return Arena; }

	UFUNCTION(BlueprintPure, Category = "Game")
	int32 GetScore(EIJPSide Side) const { return Side == EIJPSide::Left ? LeftScore : RightScore; }

protected:
	/** Pause before each serve, including the first. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Game|Serve", meta = (ClampMin = "0", Units = "s"))
	float ServeDelay = 1.f;

	/** Serves leave at a random angle within +-this from horizontal. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Game|Serve", meta = (ClampMin = "0", Units = "deg"))
	float MaxServeAngleDeg = 30.f;

	/** Controller that plays the right paddle. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Game|AI")
	TSubclassOf<AIJPPaddleAIController> AIControllerClass;

	/** How the AI plays. Set in DefaultGame.ini; empty uses UIJPAIProfile's defaults. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Game|AI")
	TSoftObjectPtr<UIJPAIProfile> AIProfile;

private:
	void PossessPlayerPaddle(APlayerController* PlayerController);
	void SpawnAIPaddle(EIJPSide Side);
	void ScheduleServe(EIJPSide Toward);
	void ServeBall();

	UFUNCTION()
	void HandleGoal(EIJPSide DefendingSide);

	FTimerHandle ServeTimer;
	EIJPSide NextServeSide = EIJPSide::Left;
	int32 LeftScore = 0;
	int32 RightScore = 0;

	UPROPERTY(Transient)
	TObjectPtr<AIJPArena> Arena;
};
