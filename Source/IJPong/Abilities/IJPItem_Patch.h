// It's Just Pong

#pragma once

#include "CoreMinimal.h"
#include "Abilities/IJPAbility.h"
#include "IJPItem_Patch.generated.h"

/** Item: restore some health, up to the maximum (in a run, the run's health too). */
UCLASS()
class IJPONG_API UIJPItem_Patch : public UIJPAbility
{
	GENERATED_BODY()

public:
	UIJPItem_Patch() { Cooldown = 0.f; }

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Patch", meta = (ClampMin = "0"))
	float Heal = 2.f;

	virtual void Activate() override;
};
