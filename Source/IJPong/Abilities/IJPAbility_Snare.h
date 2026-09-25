// It's Just Pong

#pragma once

#include "CoreMinimal.h"
#include "Abilities/IJPAbility.h"
#include "IJPAbility_Snare.generated.h"

/**
 * Rival ability: slows the other paddle for a moment. Its dashes shrink with it, so it's harder for
 * the Striker, but the paddle still moves and still returns: a soft counter.
 * The AI uses it as a ball heads for the other side, so the slow lands while they're chasing it.
 */
UCLASS()
class IJPONG_API UIJPAbility_Snare : public UIJPAbility
{
	GENERATED_BODY()

public:
	UIJPAbility_Snare() { Cooldown = 8.f; Telegraph = 0.5f; }

	/** The other paddle's top speed while snared, as a multiple of its normal speed. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Snare", meta = (ClampMin = "0", ClampMax = "1"))
	float SpeedMultiplier = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Snare", meta = (ClampMin = "0", Units = "s"))
	float Duration = 1.5f;

	virtual void Activate() override;
	virtual bool IsActive() const override { return TimeLeft > 0.f; }
	virtual bool WantsAIUse() const override;
	virtual void TickAbility(float DeltaSeconds) override;
	virtual void Deactivate() override;

private:
	AIJPPaddle* GetTarget() const;

	TWeakObjectPtr<AIJPPaddle> Snared;
	float TimeLeft = 0.f;
};
