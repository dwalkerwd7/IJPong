// It's Just Pong

#pragma once

#include "CoreMinimal.h"
#include "Abilities/IJPAbility.h"
#include "IJPAbility_Catch.generated.h"

class AIJPBall;

/**
 * The Catcher's class skill. Arm it, and the paddle's next return is caught instead: the ball
 * sticks to the paddle (AIJPBall::Hold), which keeps moving while the player aims (right stick or
 * mouse; the aim starts at the angle the return would have had). Letting go of the button fires it
 * at the aim, as fast as the return would have been; after HoldTime it fires on its own. The aim
 * may go steeper than a bounce (AimLimitDeg), for bank shots off the walls.
 */
UCLASS()
class IJPONG_API UIJPAbility_Catch : public UIJPAbility
{
	GENERATED_BODY()

public:
	UIJPAbility_Catch() { Cooldown = 6.f; }

	/** Longest the ball can be held before it fires itself. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Catch", meta = (ClampMin = "0", Units = "s"))
	float HoldTime = 1.5f;

	/** Steepest aim, in degrees from straight ahead. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Catch", meta = (ClampMin = "0", ClampMax = "85"))
	float AimLimitDeg = 70.f;

	/** Length of the aim line shown while holding. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Catch", meta = (ClampMin = "0"))
	float AimLineLength = 60.f;

	virtual void Activate() override { bArmed = true; }
	virtual bool IsActive() const override { return bArmed || Held.IsValid(); }
	virtual bool IsArmed() const override { return bArmed; }
	virtual void OnBallHit(AIJPBall& Ball) override;
	virtual void OnButtonReleased() override;
	virtual void TickAbility(float DeltaSeconds) override;
	virtual void Deactivate() override;

	UFUNCTION(BlueprintPure, Category = "Catch")
	bool IsHolding() const { return Held.IsValid(); }

	/** The aim the ball would leave at now: the paddle's aim, within AimLimitDeg. */
	float GetFireAngle() const;

private:
	void Fire();

	TWeakObjectPtr<AIJPBall> Held;
	float HoldLeft = 0.f;
	float FireSpeed = 0.f;
	bool bArmed = false;
};
