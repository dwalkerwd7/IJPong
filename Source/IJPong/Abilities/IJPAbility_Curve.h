// It's Just Pong

#pragma once

#include "CoreMinimal.h"
#include "Abilities/IJPAbility.h"
#include "IJPAbility_Curve.generated.h"

/**
 * Arm the paddle: its next return bends mid-flight (AIJPBall::Curve), toward the way the paddle
 * was moving when it hit. Hit it standing still and it bends back against its own slope.
 */
UCLASS()
class IJPONG_API UIJPAbility_Curve : public UIJPAbility
{
	GENERATED_BODY()

public:
	UIJPAbility_Curve() { Cooldown = 5.f; }

	/** How fast the path turns. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Curve", meta = (ClampMin = "0", Units = "deg"))
	float DegreesPerSecond = 70.f;

	/** How long it keeps turning after the hit. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Curve", meta = (ClampMin = "0", Units = "s"))
	float Duration = 0.9f;

	virtual void Activate() override { bArmed = true; }
	virtual bool IsActive() const override { return bArmed; }
	virtual bool IsArmed() const override { return bArmed; }
	virtual void OnBallHit(AIJPBall& Ball) override;
	virtual void Deactivate() override { bArmed = false; }

private:
	bool bArmed = false;
};
