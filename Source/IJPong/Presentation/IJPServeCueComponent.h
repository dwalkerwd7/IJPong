// It's Just Pong

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "IJPServeCueComponent.generated.h"

class AIJPBall;
class UInstancedStaticMeshComponent;

/**
 * The cue before a ball comes into play: the ball waits at the centre court while a ring of
 * segments fills around it; when the ring is full the ball blinks with a beep for BlinkTime, then
 * it's served (by whoever started the cue, at Duration). Hides itself once that ball is served or
 * taken away.
 */
UCLASS(ClassGroup = (IJPong))
class IJPONG_API UIJPServeCueComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	UIJPServeCueComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Bring Ball in over Duration seconds: the ring fills, then the ball blinks and beeps. */
	void Start(AIJPBall* Ball, float Duration);

	void Stop();

	bool IsShowing() const { return Ball.IsValid(); }

	/** How full the ring is (0..1). */
	float GetProgress() const;

	UPROPERTY(EditAnywhere, Category = "Serve Cue", meta = (ClampMin = "4"))
	int32 Segments = 24;

	UPROPERTY(EditAnywhere, Category = "Serve Cue", meta = (ClampMin = "1"))
	float Radius = 26.f;

	/** The blink before the serve; at most 40% of the whole wait. */
	UPROPERTY(EditAnywhere, Category = "Serve Cue", meta = (ClampMin = "0", Units = "s"))
	float BlinkTime = 0.35f;

private:
	void DrawRing(int32 Lit);

	UPROPERTY(Transient)
	TObjectPtr<UInstancedStaticMeshComponent> Ring;

	TWeakObjectPtr<AIJPBall> Ball;
	float Elapsed = 0.f;
	float FillTime = 0.f;
	bool bBlinking = false;
	int32 DrawnLit = -1;
};
