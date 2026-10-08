// It's Just Pong

#pragma once

#include "CoreMinimal.h"
#include "Abilities/IJPAbility.h"
#include "IJPAbility_Bumpers.generated.h"

class AIJPBonusTarget;

/**
 * A boss skill (Tilt, the Arcade boss): drops pinball bumpers mid-court for a while. A ball bounces
 * off a bumper with a kick (AIJPBonusTarget bumper mode). The AI drops them when there are none up
 * and a ball is in play.
 */
UCLASS()
class IJPONG_API UIJPAbility_Bumpers : public UIJPAbility
{
	GENERATED_BODY()

public:
	UIJPAbility_Bumpers() { Cooldown = 10.f; }

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bumpers", meta = (ClampMin = "1", ClampMax = "8"))
	int32 Count = 3;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bumpers", meta = (ClampMin = "4"))
	float Size = 24.f;

	/** How long they stay. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bumpers", meta = (ClampMin = "0", Units = "s"))
	float Lifetime = 6.f;

	/** A ball leaves a bumper this many times faster. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bumpers", meta = (ClampMin = "1"))
	float Boost = 1.15f;

	/** How far out from the net they can land, as a fraction of the half-court. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bumpers", meta = (ClampMin = "0", ClampMax = "1"))
	float CourtFraction = 0.5f;

	virtual void Activate() override;
	virtual bool IsActive() const override { return !Bumpers.IsEmpty(); }
	virtual bool WantsAIUse() const override;
	virtual void TickAbility(float DeltaSeconds) override;
	virtual void Deactivate() override;

	int32 GetNumBumpers() const { return Bumpers.Num(); }

private:
	void Clear();

	UPROPERTY(Transient)
	TArray<TObjectPtr<AIJPBonusTarget>> Bumpers;

	float TimeLeft = 0.f;
};
