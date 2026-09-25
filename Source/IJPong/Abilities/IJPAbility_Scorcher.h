// It's Just Pong

#pragma once

#include "CoreMinimal.h"
#include "Abilities/IJPAbility.h"
#include "IJPAbility_Scorcher.generated.h"

/**
 * Rival ability, hard for the Catcher: for a while its returns are hot (AIJPBall::SetHeat). A hot
 * ball burns the next paddle to touch it: a catch holds it for Heat less of its hold time, so the
 * Catcher has to aim fast. Anyone else just returns it as normal. The paddle glows while it's hot.
 * The AI uses it against a catch, with a ball on its way to it.
 */
UCLASS()
class IJPONG_API UIJPAbility_Scorcher : public UIJPAbility
{
	GENERATED_BODY()

public:
	UIJPAbility_Scorcher() { Cooldown = 10.f; Telegraph = 0.5f; }

	/** How much of a catch's hold time a hot ball burns away (0..1). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Scorcher", meta = (ClampMin = "0", ClampMax = "1"))
	float Heat = 0.65f;

	/** How long its returns stay hot. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Scorcher", meta = (ClampMin = "0", Units = "s"))
	float Duration = 4.f;

	virtual void Activate() override { TimeLeft = Duration; }
	virtual bool IsActive() const override { return TimeLeft > 0.f; }
	virtual bool IsArmed() const override { return TimeLeft > 0.f; }
	virtual bool WantsAIUse() const override;
	virtual void TickAbility(float DeltaSeconds) override { TimeLeft = FMath::Max(TimeLeft - DeltaSeconds, 0.f); }
	virtual void OnBallHit(AIJPBall& Ball) override;
	virtual void Deactivate() override { TimeLeft = 0.f; }

private:
	float TimeLeft = 0.f;
};
