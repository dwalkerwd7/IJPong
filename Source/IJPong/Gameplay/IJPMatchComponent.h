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
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FIJPPointScoredSignature, EIJPSide, Scorer, int32, Points);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FIJPHealthChangedSignature, EIJPSide, Side, float, Health, float, Damage);

/**
 * Runs one match at a time on an arena: serve -> rally -> goal -> damage -> serve, until a side's
 * health runs out. Then the balls stop, the winner's number blinks, and it waits for StartMatch()
 * to begin the next one. Lives on the game mode, so one level can host many matches in a row.
 * Health is the only thing that decides a match: goals take it (the ball's points times the rules'
 * GoalDamage) and anything else can through ApplyDamage (spells). The number over each half shows
 * that side's health, rounded up, so a side is never shown at 0 while it still stands.
 * With several balls in play, each goal counts on its own and takes that ball out; the rally
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

	/** Balls launched on every serve on top of the rules' own (e.g. a run's ball rewards). Kept until changed. */
	void SetExtraServedBalls(const TArray<TObjectPtr<const UIJPBallType>>& Types) { ExtraServedBalls = Types; }

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

	/**
	 * Take health from Side (a goal against it, a spell that landed). At 0 the other side wins.
	 * Ignored when no match is being played or the rules are endless.
	 */
	UFUNCTION(BlueprintCallable, Category = "Match")
	void ApplyDamage(EIJPSide Side, float Amount);

	/** Give Side health back, up to its maximum (items like Patch). Sends OnHealthChanged with a negative Damage. */
	UFUNCTION(BlueprintCallable, Category = "Match")
	void Heal(EIJPSide Side, float Amount);

	/** Override a side's health for this match, e.g. the player's from the run. Call after StartMatch. */
	UFUNCTION(BlueprintCallable, Category = "Match")
	void SetHealth(EIJPSide Side, float InHealth, float InMaxHealth);

	UFUNCTION(BlueprintPure, Category = "Match")
	float GetHealth(EIJPSide Side) const { return Health[SideIndex(Side)]; }

	UFUNCTION(BlueprintPure, Category = "Match")
	float GetMaxHealth(EIJPSide Side) const { return MaxHealth[SideIndex(Side)]; }

	/** Goals Side has scored this match (one per ball, whatever it was worth). */
	UFUNCTION(BlueprintPure, Category = "Match")
	int32 GetGoals(EIJPSide Side) const { return Goals[SideIndex(Side)]; }

	UFUNCTION(BlueprintPure, Category = "Match")
	EIJPMatchPhase GetPhase() const { return Phase; }

	UFUNCTION(BlueprintPure, Category = "Match")
	bool IsOver() const { return Phase == EIJPMatchPhase::MatchOver; }

	/** Who won. Only meaningful once IsOver(). */
	UFUNCTION(BlueprintPure, Category = "Match")
	EIJPSide GetWinner() const { return Winner; }

	/** The rules in play: the ones passed to StartMatch, or UIJPMatchRules' defaults. */
	const UIJPMatchRules& GetRules() const;

	/** A goal was counted and the match goes on (health already taken). A match-winning goal sends OnMatchEnded instead. */
	UPROPERTY(BlueprintAssignable, Category = "Match")
	FIJPPointScoredSignature OnPointScored;

	/** A side took damage (or was healed: negative Damage); Health is what it has now. Sent before any match end the damage causes. */
	UPROPERTY(BlueprintAssignable, Category = "Match")
	FIJPHealthChangedSignature OnHealthChanged;

	/** A side's health ran out. */
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
	/** bInstant: a new match or a set health, not a hit (health bars skip their damage trail). */
	void UpdateScoreDisplay(bool bInstant = false) const;
	bool IsPlaying() const { return Phase == EIJPMatchPhase::Serve || Phase == EIJPMatchPhase::Rally; }
	static int32 SideIndex(EIJPSide Side) { return Side == EIJPSide::Left ? 0 : 1; }
	AIJPBall* GetBall() const;

	UPROPERTY(Transient)
	TObjectPtr<AIJPArena> Arena;

	UPROPERTY(Transient)
	TObjectPtr<const UIJPMatchRules> Rules;

	UPROPERTY(Transient)
	TArray<TObjectPtr<const UIJPBallType>> ExtraServedBalls;

	/** The arena whose OnBallGoal we're bound to, so a new arena rebinds cleanly. */
	TWeakObjectPtr<AIJPArena> BoundArena;

	FTimerHandle ServeTimer;
	EIJPMatchPhase Phase = EIJPMatchPhase::None;
	EIJPSide NextServeSide = EIJPSide::Left;
	EIJPSide Winner = EIJPSide::Left;
	bool bServeHeld = false;
	float Health[2] = { 0.f, 0.f };
	float MaxHealth[2] = { 0.f, 0.f };
	int32 Goals[2] = { 0, 0 };
};
