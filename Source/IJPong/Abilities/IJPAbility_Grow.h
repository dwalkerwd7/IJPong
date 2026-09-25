// It's Just Pong

#pragma once

#include "CoreMinimal.h"
#include "Abilities/IJPAbility.h"
#include "IJPAbility_Grow.generated.h"

/** The paddle gets longer for a while. */
UCLASS()
class IJPONG_API UIJPAbility_Grow : public UIJPAbility
{
	GENERATED_BODY()

public:
	UIJPAbility_Grow() { Cooldown = 10.f; }

	/** Paddle length while grown, as a multiple of its normal length. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grow", meta = (ClampMin = "1"))
	float LengthMultiplier = 2.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grow", meta = (ClampMin = "0", Units = "s"))
	float Duration = 3.f;

	virtual void Activate() override;
	virtual bool IsActive() const override { return TimeLeft > 0.f; }
	virtual void TickAbility(float DeltaSeconds) override;
	virtual void Deactivate() override;

private:
	float TimeLeft = 0.f;
};
