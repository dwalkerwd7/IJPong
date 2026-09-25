// It's Just Pong

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "IJPRival.generated.h"

class UIJPAbility;
class UIJPAIProfile;
class UIJPConversation;
class UIJPPaddleClass;

/** A moment in play a rival can react to. */
UENUM(BlueprintType)
enum class EIJPBanterEvent : uint8
{
	/** The rival scored (any goal). */
	RivalScored,
	/** The player scored (any goal). */
	PlayerScored,
	/** The rival scored the match's first goal (used instead of RivalScored when it has lines). */
	RivalScoredFirst,
	/** The player scored the match's first goal (used instead of PlayerScored when it has lines). */
	PlayerScoredFirst,
	/** The rival just reached match point (takes priority over goal lines). */
	RivalMatchPoint,
	/** The player just reached match point (takes priority over goal lines). */
	PlayerMatchPoint,
	/** A rally reached LongRallyReturns returns. */
	LongRally
};

/** What a rival may say at one kind of moment. */
USTRUCT(BlueprintType)
struct FIJPBanterLines
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Banter")
	EIJPBanterEvent Event = EIJPBanterEvent::RivalScored;

	/** One is picked at random. Mostly one rival line; a player reply can follow in the same conversation. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Banter")
	TArray<TObjectPtr<UIJPConversation>> Conversations;

	/** Chance to speak when the moment happens (0..1). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Banter", meta = (ClampMin = "0", ClampMax = "1"))
	float Chance = 0.5f;
};

/**
 * A named opponent: a paddle class (its body), its own ability, a playing style, and the
 * conversations that carry the story.
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

	/**
	 * The rival's own ability, one no player class has; it takes the class skill's slot.
	 * Empty = the class's skill (which the AI doesn't use).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rival")
	TObjectPtr<UIJPAbility> RivalSkill;

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

	/** The rival's voice when their chat bubble babbles: a multiple of the tone set's Talk pitch. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rival", meta = (ClampMin = "0.25", ClampMax = "4"))
	float VoicePitch = 0.8f;

	/** Mid-rally reactions (see UIJPBanterComponent). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rival|Banter")
	TArray<FIJPBanterLines> Banter;

	/** At least this long between two banter lines. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rival|Banter", meta = (ClampMin = "0", Units = "s"))
	float BanterCooldown = 8.f;

	/** Returns in one rally that count as a long rally. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rival|Banter", meta = (ClampMin = "1"))
	int32 LongRallyReturns = 8;

	/** The lines for Event, or null if the rival has none. */
	const FIJPBanterLines* FindBanter(EIJPBanterEvent Event) const
	{
		return Banter.FindByPredicate([Event](const FIJPBanterLines& Lines) { return Lines.Event == Event && !Lines.Conversations.IsEmpty(); });
	}
};
