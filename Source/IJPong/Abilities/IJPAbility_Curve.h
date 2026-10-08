// It's Just Pong

#pragma once

#include "CoreMinimal.h"
#include "Abilities/IJPAbility.h"
#include "IJPAbility_Curve.generated.h"

/**
 * Arm the paddle: its next return bends mid-flight (AIJPBall::Curve), toward the way the paddle
 * was moving when it hit. Hit it standing still and it bends back against its own slope.
 * Skill-tree upgrades: Rate (fraction added to the turn rate), Duration (seconds added), Hook (> 0:
 * the bend flips halfway), ExtraCurves (more returns curve per use), LateBreak (> 0: the bend waits
 * for the net).
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

	/** Bend Ball the way Paddle was moving (or against its slope if the paddle was still), like a curve shot. */
	static void CurveReturn(AIJPBall& Ball, const AIJPPaddle& Paddle, float DegreesPerSecond, float Duration, float FlipAfter = -1.f, bool bAfterNet = false);

	virtual void Activate() override;
	virtual bool IsActive() const override { return bArmed; }
	virtual bool IsArmed() const override { return bArmed; }
	virtual void OnBallHit(AIJPBall& Ball) override;
	virtual void Deactivate() override { bArmed = false; CurvesLeft = 0; }

private:
	bool bArmed = false;
	/** Returns still to curve this use (Double Curve). */
	int32 CurvesLeft = 0;
};
