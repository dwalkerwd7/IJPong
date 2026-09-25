// It's Just Pong

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Core/IJPTypes.h"
#include "IJPBossComponent.generated.h"

class AIJPArena;
class AIJPPaddle;
class UIJPMatchComponent;
class UIJPRival;

/**
 * Runs a boss rival's fight (UIJPRival::BossLength and Phases). The boss starts huge and shrinks
 * with its health; each time its health falls past a phase's threshold, play pauses for a moment,
 * it says its line, and it changes: a new skill, a new spell (charged and ready), or splitting in
 * two. Every new match starts it over in its first form. Does nothing for ordinary rivals.
 * Lives on the game mode next to the banter.
 */
UCLASS(ClassGroup = (IJPong))
class IJPONG_API UIJPBossComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	void Bind(AIJPArena* InArena, UIJPMatchComponent* InMatch);

	/** The rival now faced (null or an ordinary rival = no boss). */
	void SetBoss(const UIJPRival* InRival);

	/** Back to the first form: full size, whole, its own skill and spell, no phases entered. */
	void Restart();

	/** Phases entered so far this fight (0 = still in its first form). */
	UFUNCTION(BlueprintPure, Category = "Boss")
	int32 GetPhasesEntered() const { return NextPhase; }

	bool IsBossFight() const;

private:
	UFUNCTION()
	void HandleHealthChanged(EIJPSide Side, float Health, float Damage);

	void EnterPhase(int32 Index);
	void UpdateSize(float HealthFraction) const;
	AIJPPaddle* GetBossPaddle() const;

	TWeakObjectPtr<AIJPArena> Arena;
	TWeakObjectPtr<UIJPMatchComponent> Match;

	UPROPERTY(Transient)
	TObjectPtr<const UIJPRival> Rival;

	int32 NextPhase = 0;
};
