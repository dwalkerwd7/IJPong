// It's Just Pong

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Core/IJPTypes.h"
#include "IJPComboComponent.generated.h"

class AIJPArena;
class AIJPBall;
class AIJPPaddle;

/**
 * Combo meters on an arena (the era's FIJPCombo, the Fighting game twist). Each return adds to its
 * side's combo; any goal breaks both. A side whose combo reaches ReturnsToFill has its super ready
 * (the paddle's armed halo): its next return leaves SuperBoost times faster and deals SuperDamage
 * times the damage if it scores (AIJPBall::SetDamageScale). A new match starts both at zero.
 */
UCLASS(ClassGroup = (IJPong))
class IJPONG_API UIJPComboComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;

	int32 GetCombo(EIJPSide Side) const { return Combo[Index(Side)]; }
	bool IsSuperReady(EIJPSide Side) const { return bSuperReady[Index(Side)]; }

	/** Both meters empty. */
	void Reset();

private:
	UFUNCTION()
	void HandleReturn(AIJPBall* Ball, AIJPPaddle* Paddle);

	UFUNCTION()
	void HandleGoal(AIJPBall* Ball, EIJPSide DefendingSide);

	AIJPArena* GetArena() const;
	void SetSuperReady(EIJPSide Side, bool bReady);
	static int32 Index(EIJPSide Side) { return Side == EIJPSide::Left ? 0 : 1; }

	int32 Combo[2] = { 0, 0 };
	bool bSuperReady[2] = { false, false };
};
