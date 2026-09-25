// It's Just Pong

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Core/IJPTypes.h"
#include "Gameplay/IJPRival.h"
#include "IJPBanterComponent.generated.h"

class AIJPArena;
class AIJPBall;
class AIJPGameModeBase;
class AIJPPaddle;
class UIJPMatchComponent;

/**
 * Mid-rally banter: watches the match for moments (goals, match point, long rallies) and lets the
 * current rival react in the chat bubbles, never pausing play. Only the most important moment
 * with lines counts (match point, then first goal, then any goal); it speaks on a roll of its
 * chance, at most once per the rival's cooldown, and cuts off whatever banter was showing.
 * It never talks over the pre-match conversation (the serve is held) or after the match ends.
 */
UCLASS(ClassGroup = (IJPong))
class IJPONG_API UIJPBanterComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	/** Start listening to this arena and match. Owned by a game mode, which supplies the rival and the bubbles. */
	void Bind(AIJPArena* InArena, UIJPMatchComponent* InMatch);

	/** Banter conversations started so far (for tests and debugging). */
	int32 GetBanterCount() const { return BanterCount; }

private:
	UFUNCTION()
	void HandlePointScored(EIJPSide Scorer, int32 Points);

	UFUNCTION()
	void HandleBallReturned(AIJPBall* Ball, AIJPPaddle* Paddle);

	/** React to the first event in ByPriority that the rival has lines for (if it's time to talk). */
	void TryBanter(const TArray<EIJPBanterEvent>& ByPriority);

	AIJPGameModeBase* GetGameMode() const;

	TWeakObjectPtr<AIJPArena> Arena;
	TWeakObjectPtr<UIJPMatchComponent> Match;
	double LastBanterTime = -1.0e9;
	int32 BanterCount = 0;
};
