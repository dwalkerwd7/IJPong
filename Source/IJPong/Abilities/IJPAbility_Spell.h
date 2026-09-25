// It's Just Pong

#pragma once

#include "CoreMinimal.h"
#include "Abilities/IJPAbility.h"
#include "Gameplay/IJPSpellStrike.h"
#include "IJPAbility_Spell.generated.h"

/**
 * A spell (the Spell slot): charged by returns (ChargeCost), then cast at the other paddle's lane,
 * where it was standing at the cast (AIJPSpellStrike). Telegraphed, dodgeable, never a goal.
 * Lightning and Fireball are two assets of this one class: a short warning and a small hit, or a
 * slow travelling shot and a big one. The AI casts as a ball heads for the other side, so the target
 * has to choose between dodging and defending.
 */
UCLASS()
class IJPONG_API UIJPAbility_Spell : public UIJPAbility
{
	GENERATED_BODY()

public:
	UIJPAbility_Spell() { Cooldown = 0.f; ChargeCost = 5; }

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spell")
	FIJPStrikeSpec Strike;

	/** Strikes cast at once, centred on the target and spaced along the lane (e.g. Brickfall's three, with gaps to slip through). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spell", meta = (ClampMin = "1", ClampMax = "7"))
	int32 Strikes = 1;

	/** Distance between neighbouring strikes' centres. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spell", meta = (ClampMin = "0"))
	float StrikeSpacing = 140.f;

	virtual void Activate() override;
	virtual bool WantsAIUse() const override;

	/** The last strike cast (for tests), or null. */
	AIJPSpellStrike* GetLastStrike() const { return LastStrike.Get(); }

private:
	TWeakObjectPtr<AIJPSpellStrike> LastStrike;
};
