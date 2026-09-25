// It's Just Pong

#pragma once

#include "CoreMinimal.h"
#include "Abilities/IJPAbility.h"
#include "IJPAbility_Freeze.generated.h"

/** Every ball in play stops dead for a moment (AIJPBall::Freeze). */
UCLASS()
class IJPONG_API UIJPAbility_Freeze : public UIJPAbility
{
	GENERATED_BODY()

public:
	UIJPAbility_Freeze() { Cooldown = 10.f; }

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Freeze", meta = (ClampMin = "0", Units = "s"))
	float Duration = 0.5f;

	virtual void Activate() override;
};
