// It's Just Pong

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "IJPRival.generated.h"

class UIJPAIProfile;
class UIJPConversation;
class UIJPPaddleClass;

/**
 * A named opponent: a paddle class, a playing style, and the conversations that carry the story.
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

	/** Played before a match (one at random); the first serve waits for it. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rival|Conversations")
	TArray<TObjectPtr<UIJPConversation>> PreMatch;

	/** Played after the rival wins (one at random). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rival|Conversations")
	TArray<TObjectPtr<UIJPConversation>> Win;

	/** Played after the rival loses (one at random). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rival|Conversations")
	TArray<TObjectPtr<UIJPConversation>> Loss;
};
