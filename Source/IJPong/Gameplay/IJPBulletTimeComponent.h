// It's Just Pong

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "IJPBulletTimeComponent.generated.h"

class AIJPArena;
class AIJPBall;
class AIJPPaddle;

/**
 * Bullet time on an arena (the era's FIJPBulletTime, the Matrix twist). When a ball heading for a
 * goal is past its paddle's lane, or will reach the lane within Lead where the paddle isn't, every
 * ball slows to TimeScale for Duration (real time); paddles keep their speed, so either side gets a
 * last chance. Each ball triggers it once per approach (until it's next returned or served).
 */
UCLASS(ClassGroup = (IJPong))
class IJPONG_API UIJPBulletTimeComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UIJPBulletTimeComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** What balls' time is multiplied by now (1 = normal). */
	float GetBallTimeFactor() const { return TimeLeft > 0.f ? Factor : 1.f; }

	bool IsActive() const { return TimeLeft > 0.f; }

	/** Back to normal time, every ball able to trigger it again. */
	void Reset();

private:
	AIJPArena* GetArena() const;
	/** Ball is about to get past the paddle defending the goal it's heading for. */
	bool IsNearGoal(const AIJPBall& Ball, float Lead) const;

	/** Each ball's rally count when it last triggered, so it triggers once per approach. */
	TMap<TWeakObjectPtr<AIJPBall>, int32> Triggered;

	float TimeLeft = 0.f;
	float Factor = 1.f;
};
