// It's Just Pong

#pragma once

#include "CoreMinimal.h"
#include "Engine/Texture2D.h"
#include "IJPPortrait.generated.h"

/** A portrait's face. Mood = whatever the match says (ahead, behind, level). */
UENUM(BlueprintType)
enum class EIJPExpression : uint8
{
	Mood,
	Neutral,
	Smug,
	Rattled
};

/** One character's faces (greyscale, tinted by their side's colour). Neutral stands in for any that are missing. */
USTRUCT(BlueprintType)
struct FIJPPortraits
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Portraits")
	TObjectPtr<UTexture2D> Neutral;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Portraits")
	TObjectPtr<UTexture2D> Smug;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Portraits")
	TObjectPtr<UTexture2D> Rattled;

	bool IsSet() const { return Neutral != nullptr; }

	UTexture2D* Get(EIJPExpression Expression) const
	{
		UTexture2D* Face = Expression == EIJPExpression::Smug ? Smug.Get() : Expression == EIJPExpression::Rattled ? Rattled.Get() : nullptr;
		return Face ? Face : Neutral.Get();
	}
};
