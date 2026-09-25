// It's Just Pong

#pragma once

#include "CoreMinimal.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Presentation/IJPBlinker.h"
#include "IJPChargePipsComponent.generated.h"

/**
 * A spell's charge as a row of chunky pips (the Cabinet era's charge meter, under each side's
 * health): a full square per charge earned, a small dot for each still to earn, all blinking once
 * it's full. Nothing shows when there's no spell (Total 0). Laid out in its own X / Z plane.
 */
UCLASS(ClassGroup = (IJPong), meta = (BlueprintSpawnableComponent))
class IJPONG_API UIJPChargePipsComponent : public UInstancedStaticMeshComponent
{
	GENERATED_BODY()

public:
	UIJPChargePipsComponent();

	UFUNCTION(BlueprintCallable, Category = "Charge Pips")
	void SetCharge(int32 InFilled, int32 InTotal);

	UFUNCTION(BlueprintPure, Category = "Charge Pips")
	int32 GetFilled() const { return Filled; }

	UFUNCTION(BlueprintPure, Category = "Charge Pips")
	int32 GetTotal() const { return Total; }

	/** Full: blinking to say "ready". */
	UFUNCTION(BlueprintPure, Category = "Charge Pips")
	bool IsBlinking() const { return Blinker.IsRunning(); }

	UPROPERTY(EditAnywhere, Category = "Charge Pips", meta = (ClampMin = "1"))
	float PipSize = 12.f;

	UPROPERTY(EditAnywhere, Category = "Charge Pips", meta = (ClampMin = "0"))
	float PipSpacing = 6.f;

	/** An empty pip's size, as a fraction of a full one. */
	UPROPERTY(EditAnywhere, Category = "Charge Pips", meta = (ClampMin = "0", ClampMax = "1"))
	float EmptyScale = 0.3f;

private:
	void Rebuild();

	FIJPBlinker Blinker;
	int32 Filled = 0;
	int32 Total = 0;
};
