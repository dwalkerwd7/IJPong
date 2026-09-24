// It's Just Pong

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "IJPPaddleAIController.generated.h"

class AIJPBall;
class AIJPPaddle;
class UIJPAIProfile;

/**
 * Plays a paddle. Every ReactionTime it predicts where the ball will meet its paddle face, then
 * steers there through AddMoveInput, exactly like a player would. Imperfection comes from the
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

	/** Where the paddle is currently heading (plane Y). */
	UFUNCTION(BlueprintPure, Category = "AI")
	float GetTargetY() const { return TargetY; }

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

private:
	const UIJPAIProfile& GetProfile() const;
	void Decide(const AIJPPaddle& Paddle, const AIJPBall& Ball);
	void Steer(AIJPPaddle& Paddle) const;

	UPROPERTY(Transient)
	TObjectPtr<const UIJPAIProfile> Profile;

	FRandomStream Random;
	float Skill = 0.5f;
	float DecisionTimer = 0.f;
	float TargetY = 0.f;
	float ShotError = 0.f;
	float ShotAim = 0.f;
	bool bBallIncoming = false;
};
