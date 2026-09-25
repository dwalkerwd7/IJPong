// It's Just Pong

#pragma once

#include "CoreMinimal.h"
#include "Abilities/IJPAbility.h"
#include "IJPAbility_Reader.generated.h"

class AIJPBall;

/**
 * Rival ability, hard for the Catcher: for a while it reads a held ball. While the other paddle holds
 * one, it works out where the shot would land on its side if fired at the current aim, and its AI
 * heads there before the ball is even released (AIJPPaddleAIController::SetReadTarget). The longer
 * the aim, the better placed it is; a quick release still beats it. The AI uses it when the other
 * side arms a catch or is holding a ball.
 */
UCLASS()
class IJPONG_API UIJPAbility_Reader : public UIJPAbility
{
	GENERATED_BODY()

public:
	UIJPAbility_Reader() { Cooldown = 8.f; Telegraph = 0.5f; }

	/** How long it keeps reading. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reader", meta = (ClampMin = "0", Units = "s"))
	float Duration = 3.f;

	virtual void Activate() override { TimeLeft = Duration; }
	virtual bool IsActive() const override { return TimeLeft > 0.f; }
	virtual bool WantsAIUse() const override;
	virtual void TickAbility(float DeltaSeconds) override;
	virtual void Deactivate() override;

	/** Where the held shot would reach this paddle's face, or false if nothing's held. */
	bool PredictHeldShot(float& OutY) const;

private:
	AIJPPaddle* GetTarget() const;
	AIJPBall* FindHeldBall() const;
	void StopReading();

	float TimeLeft = 0.f;
};
