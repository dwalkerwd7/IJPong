// It's Just Pong

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "IJPToneSet.generated.h"

/** One beep. */
USTRUCT(BlueprintType)
struct FIJPTone
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tone", meta = (ClampMin = "20", ClampMax = "20000", Units = "Hz"))
	float Frequency = 440.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tone", meta = (ClampMin = "0", Units = "s"))
	float Duration = 0.03f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tone", meta = (ClampMin = "0", ClampMax = "1"))
	float Volume = 0.25f;

	bool operator==(const FIJPTone& Other) const
	{
		return Frequency == Other.Frequency && Duration == Other.Duration && Volume == Other.Volume;
	}
};

/** The beeps the game makes. Defaults approximate the 1972 cabinet; other sets (detuned, broken...) are just other assets. */
UCLASS(BlueprintType)
class IJPONG_API UIJPToneSet : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tones")
	FIJPTone PaddleHit = { 490.f, 0.035f, 0.25f };

	/** Walls, and the top/bottom edges of paddles. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tones")
	FIJPTone Bounce = { 245.f, 0.035f, 0.25f };

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tones")
	FIJPTone Goal = { 122.f, 0.35f, 0.3f };

	/** Arming a skill (Smash, Curve shot, Split): this, then a second note ArmRise times higher. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tones")
	FIJPTone Arm = { 740.f, 0.04f, 0.15f };

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tones", meta = (ClampMin = "0.1"))
	float ArmRise = 1.5f;

	/** The blip of chat-bubble text typing out (the opponent's is pitched a little lower). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tones")
	FIJPTone Talk = { 660.f, 0.015f, 0.1f };
};
