// It's Just Pong

#pragma once

#include "CoreMinimal.h"
#include "Abilities/IJPAbility.h"
#include "IJPAbility_Magnet.generated.h"

class AIJPBall;

/**
 * Rival ability: for a moment, every ball heading this way is pulled in: its curve bends less and a
 * smash loses some of its extra speed (AIJPBall::Dampen, once per ball). Softens the Classic's Smash
 * and the Curver's curve shot without cancelling them. The AI uses it on a curving or smashed ball
 * coming at it.
 */
UCLASS()
class IJPONG_API UIJPAbility_Magnet : public UIJPAbility
{
	GENERATED_BODY()

public:
	UIJPAbility_Magnet() { Cooldown = 8.f; Telegraph = 0.5f; }

	/** A pulled ball's curve turns this fraction as fast. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Magnet", meta = (ClampMin = "0", ClampMax = "1"))
	float CurveScale = 0.5f;

	/** A pulled smash keeps this fraction of its extra speed. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Magnet", meta = (ClampMin = "0", ClampMax = "1"))
	float BoostScale = 0.5f;

	/** How long the pull lasts, catching any ball that comes this way meanwhile. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Magnet", meta = (ClampMin = "0", Units = "s"))
	float Duration = 1.f;

	virtual void Activate() override;
	virtual bool IsActive() const override { return TimeLeft > 0.f; }
	virtual bool WantsAIUse() const override;
	virtual void TickAbility(float DeltaSeconds) override;
	virtual void Deactivate() override;

private:
	/** Dampen each in-play ball heading this way that hasn't been pulled yet. */
	void Pull();
	bool IsIncoming(const AIJPBall& Ball) const;

	TArray<TWeakObjectPtr<AIJPBall>> Pulled;
	float TimeLeft = 0.f;
};
