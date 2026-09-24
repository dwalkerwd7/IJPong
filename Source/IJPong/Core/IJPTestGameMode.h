// It's Just Pong

#pragma once

#include "CoreMinimal.h"
#include "Core/IJPGameModeBase.h"
#include "IJPTestGameMode.generated.h"

/**
 * Endless play for trying out a level: serve -> goal -> score -> serve, forever, no win condition.
 * Also has test tools (bound to debug keys by AIJPTestPlayerController): reset the score,
 * serve now, hand the player's paddle to an AI to watch the level play itself, and nudge the
 * opponent's skill up and down.
 */
UCLASS()
class IJPONG_API AIJPTestGameMode : public AIJPGameModeBase
{
	GENERATED_BODY()

public:
	AIJPTestGameMode();

	UFUNCTION(BlueprintPure, Category = "Test")
	int32 GetScore(EIJPSide Side) const { return Side == EIJPSide::Left ? LeftScore : RightScore; }

	/** Zero both scores and start a fresh serve after the usual delay. */
	UFUNCTION(BlueprintCallable, Category = "Test")
	void ResetScore();

	/** Serve immediately toward a random side, abandoning any rally in progress. */
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

protected:
	virtual void OnArenaReady() override;

	/** Pause before each serve, including the first. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Test|Serve", meta = (ClampMin = "0", Units = "s"))
	float ServeDelay = 1.f;

	/** Serves leave at a random angle within +-this from horizontal. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Test|Serve", meta = (ClampMin = "0", Units = "deg"))
	float MaxServeAngleDeg = 30.f;

private:
	UFUNCTION()
	void HandleGoal(EIJPSide DefendingSide);

	void ScheduleServe(EIJPSide Toward);
	void ServeBall();
	void UpdateScoreDisplay() const;
	static EIJPSide RandomSide() { return FMath::RandBool() ? EIJPSide::Left : EIJPSide::Right; }

	UPROPERTY(Transient)
	TObjectPtr<AIJPPaddleAIController> PlayerSideAI;

	FTimerHandle ServeTimer;
	EIJPSide NextServeSide = EIJPSide::Left;
	int32 LeftScore = 0;
	int32 RightScore = 0;
};
