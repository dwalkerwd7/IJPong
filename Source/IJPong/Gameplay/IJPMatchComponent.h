// It's Just Pong

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Core/IJPTypes.h"
#include "IJPMatchComponent.generated.h"

class AIJPArena;
class AIJPBall;
class UIJPBallType;
class UIJPMatchRules;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FIJPMatchEndedSignature, EIJPSide, Winner);

/**
 * Runs one match at a time on an arena: serve -> rally -> goal -> score -> serve, until a side
 * reaches the rules' win target. Then the balls stop, the winner's score blinks, and it waits
 * for StartMatch() to begin the next one. Lives on the game mode, so one level can host many
 * matches in a row.
 * With several balls in play, each goal scores that ball's points and takes it out; the rally
 * goes on until the court is empty, then the next serve follows.
 */
UCLASS(ClassGroup = (IJPong))
class IJPONG_API UIJPMatchComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	/**
	 * Start a fresh match: scores to zero, first serve after the rules' delay.
	 * Abandons any match or rally in progress. Null Rules uses UIJPMatchRules' defaults.
	 * bHoldServe: the ball blinks at the centre but waits for ReleaseServe() (e.g. a pre-match conversation).
	 */
	UFUNCTION(BlueprintCallable, Category = "Match")
	void StartMatch(AIJPArena* InArena, const UIJPMatchRules* InRules, bool bHoldServe = false);

	/** Abandon the match: no winner, balls out of play, nothing more happens until StartMatch. */
	UFUNCTION(BlueprintCallable, Category = "Match")
	void StopMatch();

	/** Let a held serve go: it follows after the rules' serve delay. */
	UFUNCTION(BlueprintCallable, Category = "Match")
	void ReleaseServe();

	UFUNCTION(BlueprintPure, Category = "Match")
	bool IsServeHeld() const { return bServeHeld; }

	/** Serve immediately toward a random side, abandoning any rally in progress. Does nothing once the match is over. */
	UFUNCTION(BlueprintCallable, Category = "Match")
	void ServeNow();

	/**
	 * Launch one more ball from the centre toward a random side, joining the rally (for abilities,
	 * boss patterns, the test mode's debug key). Null Type = the arena's default. Returns the ball,
	 * or null when no match is being played.
	 */
	UFUNCTION(BlueprintCallable, Category = "Match")
	AIJPBall* LaunchExtraBall(const UIJPBallType* Type);

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
	void HandleGoal(AIJPBall* ScoringBall, EIJPSide DefendingSide);

	/** The type of the Index-th ball served (arena default when the list is shorter or the entry empty). */
	const UIJPBallType* GetServedType(int32 Index) const;
	float RandomServeAngle() const;

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

	/** The arena whose OnBallGoal we're bound to, so a new arena rebinds cleanly. */
	TWeakObjectPtr<AIJPArena> BoundArena;

	FTimerHandle ServeTimer;
	EIJPMatchPhase Phase = EIJPMatchPhase::None;
	EIJPSide NextServeSide = EIJPSide::Left;
	EIJPSide Winner = EIJPSide::Left;
	bool bServeHeld = false;
	int32 LeftScore = 0;
	int32 RightScore = 0;
};
