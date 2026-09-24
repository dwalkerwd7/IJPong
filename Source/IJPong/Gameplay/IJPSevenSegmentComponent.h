// It's Just Pong

#pragma once

#include "CoreMinimal.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Presentation/IJPBlinker.h"
#include "IJPSevenSegmentComponent.generated.h"

/**
 * Draws a non-negative number as chunky seven-segment digits, one cube instance per lit segment.
 * Laid out in its own X (horizontal) / Z (vertical) plane, centred on the component origin.
 */
UCLASS(ClassGroup = (IJPong), meta = (BlueprintSpawnableComponent))
class IJPONG_API UIJPSevenSegmentComponent : public UInstancedStaticMeshComponent
{
	GENERATED_BODY()

public:
	UIJPSevenSegmentComponent();

	UFUNCTION(BlueprintCallable, Category = "Seven Segment")
	void SetValue(int32 NewValue);

	UFUNCTION(BlueprintPure, Category = "Seven Segment")
	int32 GetValue() const { return Value; }

	/** Blink the whole number off and on NumFlashes times, ending shown. */
	UFUNCTION(BlueprintCallable, Category = "Seven Segment")
	void Flash(int32 NumFlashes = 3, float Period = 0.12f);

	UFUNCTION(BlueprintPure, Category = "Seven Segment")
	bool IsFlashing() const { return Blinker.IsRunning(); }

	/** Width (X) and height (Z) of one digit. */
	UPROPERTY(EditAnywhere, Category = "Seven Segment")
	FVector2D DigitSize = FVector2D(48.f, 80.f);

	UPROPERTY(EditAnywhere, Category = "Seven Segment")
	float SegmentThickness = 12.f;

	/** Gap between digits. */
	UPROPERTY(EditAnywhere, Category = "Seven Segment")
	float DigitSpacing = 16.f;

	/** Thickness along Y (toward the camera). */
	UPROPERTY(EditAnywhere, Category = "Seven Segment")
	float Depth = 10.f;

protected:
	virtual void OnRegister() override;

private:
	void Rebuild();

	UPROPERTY(EditAnywhere, Category = "Seven Segment", meta = (ClampMin = "0"))
	int32 Value = 0;

	FIJPBlinker Blinker;
};
