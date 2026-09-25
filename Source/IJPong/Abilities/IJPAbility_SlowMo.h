// It's Just Pong

#pragma once

#include "CoreMinimal.h"
#include "Abilities/IJPAbility.h"
#include "IJPAbility_SlowMo.generated.h"

/** Every ball in play moves slower for a while; the paddles don't (AIJPBall::SetTimeScale). */
UCLASS()
class IJPONG_API UIJPAbility_SlowMo : public UIJPAbility
{
	GENERATED_BODY()

public:
	UIJPAbility_SlowMo() { Cooldown = 12.f; }

	/** Ball speed while it lasts, as a fraction of normal. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Slow-mo", meta = (ClampMin = "0.05", ClampMax = "1"))
	float TimeScale = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Slow-mo", meta = (ClampMin = "0", Units = "s"))
	float Duration = 2.f;

	virtual void Activate() override;
	virtual bool IsActive() const override { return TimeLeft > 0.f; }
	virtual void TickAbility(float DeltaSeconds) override;
	virtual void Deactivate() override;

private:
	void SetBallsTimeScale(float Scale) const;

	float TimeLeft = 0.f;
};
