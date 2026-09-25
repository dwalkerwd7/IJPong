// It's Just Pong

#pragma once

#include "CoreMinimal.h"
#include "Abilities/IJPAbility.h"
#include "IJPAbility_Dash.generated.h"

/** An instant burst of movement the way the paddle is being steered (AIJPPaddle::Dash). */
UCLASS()
class IJPONG_API UIJPAbility_Dash : public UIJPAbility
{
	GENERATED_BODY()

public:
	UIJPAbility_Dash() { Cooldown = 3.f; }

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dash", meta = (ClampMin = "0"))
	float Distance = 120.f;

	/** How long the burst takes. Short = closer to a teleport. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dash", meta = (ClampMin = "0.01", Units = "s"))
	float Duration = 0.08f;

	virtual void Activate() override;
};
