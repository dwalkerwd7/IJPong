// It's Just Pong

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "IJPConversation.generated.h"

/** Who says a line: the player's paddle or the opponent's. */
UENUM(BlueprintType)
enum class EIJPSpeaker : uint8
{
	Player,
	Opponent
};

USTRUCT(BlueprintType)
struct FIJPConversationLine
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Line")
	EIJPSpeaker Speaker = EIJPSpeaker::Opponent;

	/** Shown in the speaker's chat bubble. Line breaks are kept (bubbles don't wrap on their own). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Line", meta = (MultiLine = "true"))
	FText Text;

	/** Seconds the full line stays up once typed out. 0 = a readable time from its length. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Line", meta = (ClampMin = "0", Units = "s"))
	float HoldTime = 0.f;
};

/** A short exchange of lines between the two paddles, shown in their chat bubbles in order. */
UCLASS(BlueprintType)
class IJPONG_API UIJPConversation : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Conversation")
	TArray<FIJPConversationLine> Lines;

	/** One at random from Options, or null if there are none. */
	static const UIJPConversation* PickRandom(const TArray<TObjectPtr<UIJPConversation>>& Options)
	{
		return Options.IsEmpty() ? nullptr : Options[FMath::RandHelper(Options.Num())].Get();
	}
};
