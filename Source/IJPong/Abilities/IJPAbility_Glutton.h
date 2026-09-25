// It's Just Pong

#pragma once

#include "CoreMinimal.h"
#include "Abilities/IJPAbility.h"
#include "IJPAbility_Glutton.generated.h"

/**
 * Rival ability: for a moment its mouth is open, and while two or more balls are in play it
 * swallows one coming at it on its half (out of play, no goal). One ball per use: the others still
 * count, so it's harder for the Splitter but a split still pays. The AI opens up when several balls
 * are in play and one is heading its way.
 */
UCLASS()
class IJPONG_API UIJPAbility_Glutton : public UIJPAbility
{
	GENERATED_BODY()

public:
	UIJPAbility_Glutton() { Cooldown = 8.f; Telegraph = 0.5f; }

	/** How long it waits for a ball to swallow before giving up. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Glutton", meta = (ClampMin = "0", Units = "s"))
	float Duration = 2.f;

	virtual void Activate() override;
	virtual bool IsActive() const override { return TimeLeft > 0.f; }
	virtual bool IsArmed() const override { return TimeLeft > 0.f; }
	virtual bool WantsAIUse() const override;
	virtual void TickAbility(float DeltaSeconds) override;
	virtual void Deactivate() override { TimeLeft = 0.f; }

	/** Balls swallowed since equipped (for tests and the debug overlay). */
	int32 GetSwallowed() const { return Swallowed; }

private:
	/** The ball to swallow now, or null: only with two or more in play, one on this half heading here. */
	AIJPBall* FindMeal() const;

	float TimeLeft = 0.f;
	int32 Swallowed = 0;
};
