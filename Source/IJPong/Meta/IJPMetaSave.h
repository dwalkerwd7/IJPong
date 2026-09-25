// It's Just Pong

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "IJPMetaSave.generated.h"

/** What persists between runs and sessions: the meta currencies (later: skill trees, eras, unlocks). */
UCLASS()
class IJPONG_API UIJPMetaSave : public USaveGame
{
	GENERATED_BODY()

public:
	/** Earned by every run, by how deep it got. Spent on the skill trees. */
	UPROPERTY()
	int32 SkillPoints = 0;

	/** Earned by beating bosses. Spent on the skill trees. */
	UPROPERTY()
	int32 BossTokens = 0;

	/** Skill-tree nodes bought, as "<tree asset name>/<node id>". */
	UPROPERTY()
	TArray<FString> OwnedNodes;

	/** Eras unlocked, counting from the first (the Cabinet is always unlocked). */
	UPROPERTY()
	int32 ErasUnlocked = 1;

	/** Eras whose title card has been seen (so it can be skipped). */
	UPROPERTY()
	TArray<FString> SeenEraCards;

	/** The Spell slot is open, for every class (bought once with boss tokens). */
	UPROPERTY()
	bool bSpellSlotUnlocked = false;
};
