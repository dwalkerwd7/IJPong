// It's Just Pong

#pragma once

#include "CoreMinimal.h"
#include "Abilities/IJPAbility.h"
#include "IJPAbility_Barrier.generated.h"

class AIJPBall;

/**
 * A shield line in front of the paddle's own goal for a while (AIJPArena::SetBarrierUp).
 * Skill-tree upgrades: Duration (seconds added), LastStand (> 0: once per match the barrier rises by
 * itself when a ball gets past the paddle), Mending (health healed per ball the barrier blocks),
 * Rebound (> 0: blocked balls come back boosted).
 */
UCLASS()
class IJPONG_API UIJPAbility_Barrier : public UIJPAbility
{
	GENERATED_BODY()

public:
	UIJPAbility_Barrier() { Cooldown = 12.f; }

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Barrier", meta = (ClampMin = "0", Units = "s"))
	float Duration = 2.5f;

	/** Last Stand: how long the barrier stays up when it rises by itself. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Barrier|Upgrades", meta = (ClampMin = "0", Units = "s"))
	float LastStandTime = 1.f;

	/** Rebound: a blocked ball leaves this many times faster. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Barrier|Upgrades", meta = (ClampMin = "1"))
	float ReboundBoost = 1.3f;

	virtual void Activate() override;
	virtual bool IsActive() const override { return TimeLeft > 0.f; }
	virtual void TickAbility(float DeltaSeconds) override;
	virtual void Deactivate() override;

	/** Last Stand has been used in the match being played. */
	bool HasUsedLastStand() const;

private:
	UFUNCTION()
	void HandleBarrierHit(EIJPSide Side, AIJPBall* Ball);

	void Raise(float Seconds);
	void SetUp(bool bUp);
	/** A ball is past (or about to get past) the paddle on its way into this goal. */
	bool IsGoalComing() const;
	int32 GetMatchNumber() const;

	float TimeLeft = 0.f;
	/** The match Last Stand was used in (0 = none yet). */
	int32 LastStandMatch = 0;
};
