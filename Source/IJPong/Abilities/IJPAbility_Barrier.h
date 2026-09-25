// It's Just Pong

#pragma once

#include "CoreMinimal.h"
#include "Abilities/IJPAbility.h"
#include "IJPAbility_Barrier.generated.h"

/** A shield line in front of the paddle's own goal for a while (AIJPArena::SetBarrierUp). */
UCLASS()
class IJPONG_API UIJPAbility_Barrier : public UIJPAbility
{
	GENERATED_BODY()

public:
	UIJPAbility_Barrier() { Cooldown = 12.f; }

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Barrier", meta = (ClampMin = "0", Units = "s"))
	float Duration = 2.5f;

	virtual void Activate() override;
	virtual bool IsActive() const override { return TimeLeft > 0.f; }
	virtual void TickAbility(float DeltaSeconds) override;
	virtual void Deactivate() override;

private:
	void SetUp(bool bUp) const;

	float TimeLeft = 0.f;
};
