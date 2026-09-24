// It's Just Pong

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Core/IJPTypes.h"
#include "IJPMatchComponent.generated.h"

class AIJPArena;
class AIJPBall;
class UIJPMatchRules;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FIJPMatchEndedSignature, EIJPSide, Winner);

/**
 * Runs one match at a time on an arena: serve -> rally -> goal -> score -> serve, until a side
 * reaches the rules' win target. Then the ball stops, the winner's score blinks, and it waits
 * for StartMatch() to begin the next one. Lives on the game mode, so one level can host many
 * matches in a row.
 */
UCLASS(ClassGroup = (IJPong))
class IJPONG_API UIJPMatchComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	/**
	 * Start a fresh match: scores to zero, first serve after the rules' delay.
	 * Abandons any match or rally in progress. Null Rules uses UIJPMatchRules' defaults.
	 */
	UFUNCTION(BlueprintCallable, Category = "Match")
	void StartMatch(AIJPArena* InArena, const UIJPMatchRules* InRules);

	/** Serve immediately toward a random side, abandoning any rally in progress. Does nothing once the match is over. */
	UFUNCTION(BlueprintCallable, Category = "Match")
	void ServeNow();

	UFUNCTION(BlueprintPure, Category = "Match")
	int32 GetScore(EIJPSide Side) const { return Side == EIJPSide::Left ? LeftScore : RightScore; }

	UFUNCTION(BlueprintPure, Category = "Match")
	EIJPMatchPhase GetPhase() const { return Phase; }

	UFUNCTION(BlueprintPure, Category = "Match")
	bool IsOver() const { return Phase == EIJPMatchPhase::MatchOver; }

	/** Who won. Only meaningful once IsOver(). */
	UFUNCTION(BlueprintPure, Category = "Match")
	EIJPSide GetWinner() const { return Winner; }

	/** The rules in play: the ones passed to StartMatch, or UIJPMatchRules' defaults. */
	const UIJPMatchRules& GetRules() const;

	/** A side reached the win target. */
	UPROPERTY(BlueprintAssignable, Category = "Match")
	FIJPMatchEndedSignature OnMatchEnded;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UFUNCTION()
	void HandleGoal(EIJPSide DefendingSide);

	void ScheduleServe(EIJPSide Toward);
	void ServeBall();
	void Serve(EIJPSide Toward);
	void EndMatch(EIJPSide InWinner);
	void UpdateScoreDisplay() const;
	AIJPBall* GetBall() const;

	UPROPERTY(Transient)
	TObjectPtr<AIJPArena> Arena;

	UPROPERTY(Transient)
	TObjectPtr<const UIJPMatchRules> Rules;

	/** The ball whose OnGoal we're bound to, so a new arena or ball rebinds cleanly. */
	TWeakObjectPtr<AIJPBall> BoundBall;

	FTimerHandle ServeTimer;
	EIJPMatchPhase Phase = EIJPMatchPhase::None;
	EIJPSide NextServeSide = EIJPSide::Left;
	EIJPSide Winner = EIJPSide::Left;
	int32 LeftScore = 0;
	int32 RightScore = 0;
};
