// It's Just Pong

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "IJPRival.generated.h"

class UIJPAIProfile;
class UIJPPaddleClass;

/**
 * A named opponent: a paddle class, a playing style, and a few lines that carry the story.
 * How hard they play is not theirs to decide: that's authored per arena (OpponentSkill).
 */
UCLASS(BlueprintType)
class IJPONG_API UIJPRival : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rival")
	FText DisplayName;

	/** Empty = the arena's configured class for that side. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rival")
	TObjectPtr<UIJPPaddleClass> PaddleClass;

	/** How they play (reaction, error, aim). Empty = the game mode's AIProfile. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rival")
	TObjectPtr<UIJPAIProfile> AIProfile;

	/** Said before a match; one is picked. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rival|Lines", meta = (MultiLine = "true"))
	TArray<FText> PreMatchLines;

	/** Said after the rival wins. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rival|Lines", meta = (MultiLine = "true"))
	TArray<FText> WinLines;

	/** Said after the rival loses. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rival|Lines", meta = (MultiLine = "true"))
	TArray<FText> LossLines;
};
