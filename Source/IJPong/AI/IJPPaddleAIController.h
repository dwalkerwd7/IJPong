// It's Just Pong

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "IJPPaddleAIController.generated.h"

class AIJPBall;
class AIJPPaddle;
class UIJPAIProfile;

/**
 * Plays a paddle. It also uses the paddle's abilities when they say it's their moment (WantsAIUse). Every ReactionTime it picks the ball that will reach its paddle first, predicts
 * where that ball will meet its paddle face, then steers there through AddMoveInput, exactly like a
 * player would. Imperfection comes from the
 * profile: a speed-scaled misjudgement and an aim offset, both rolled once per incoming shot.
 * An AAIController so behaviour trees can be layered on later (e.g. deciding when to use abilities);
 * this positioning logic would then become the tree's "defend" behaviour.
 */
UCLASS()
class IJPONG_API AIJPPaddleAIController : public AAIController
{
	GENERATED_BODY()

public:
	AIJPPaddleAIController();

	virtual void Tick(float DeltaSeconds) override;

	/** The profile in use: the one set, or UIJPAIProfile's defaults. */
	const UIJPAIProfile& GetProfile() const;

	/** Null falls back to UIJPAIProfile's defaults. */
	UFUNCTION(BlueprintCallable, Category = "AI")
	void SetProfile(const UIJPAIProfile* InProfile);

	/** Where on the difficulty spectrum this opponent plays: 0 = hopeless, 1 = near-perfect. */
	UFUNCTION(BlueprintCallable, Category = "AI")
	void SetSkill(float InSkill) { Skill = FMath::Clamp(InSkill, 0.f, 1.f); }

	UFUNCTION(BlueprintPure, Category = "AI")
	float GetSkill() const { return Skill; }

	/** For repeatable tests. */
	void SetRandomSeed(int32 Seed) { Random.Initialize(Seed); }

	/**
	 * Head for Y regardless of the balls, until ClearReadTarget. For abilities that let the AI see a
	 * shot coming before it's played (a rival's Reader).
	 */
	void SetReadTarget(float Y) { ReadTarget = Y; bHasReadTarget = true; }

	void ClearReadTarget();

	bool HasReadTarget() const { return bHasReadTarget; }

	/** Where the paddle is currently heading (plane Y). */
	UFUNCTION(BlueprintPure, Category = "AI")
	float GetTargetY() const { return TargetY; }

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

private:
	/** The in-play ball heading for this paddle that will reach it soonest, or null. */
	const AIJPBall* PickIncomingBall(const AIJPPaddle& Paddle) const;
	void Decide(const AIJPPaddle& Paddle, const AIJPBall* Ball);
	void Steer(AIJPPaddle& Paddle) const;
	/** Target, moved out of any spell zone about to land on this lane. */
	float AvoidStrikes(const AIJPPaddle& Paddle, float Target) const;
	/** Trigger any ability that says now is its moment (rival abilities). */
	void UseAbilities(AIJPPaddle& Paddle) const;

	UPROPERTY(Transient)
	TObjectPtr<const UIJPAIProfile> Profile;

	FRandomStream Random;
	float Skill = 0.5f;
	float DecisionTimer = 0.f;
	float TargetY = 0.f;
	float ShotError = 0.f;
	float ShotAim = 0.f;
	bool bBallIncoming = false;
	bool bHasReadTarget = false;
	float ReadTarget = 0.f;

	/** The ball the current shot's error and aim were rolled for. */
	TWeakObjectPtr<const AIJPBall> TrackedBall;
};
