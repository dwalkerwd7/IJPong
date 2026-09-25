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
};
