// It's Just Pong

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "IJPDriftingBlockComponent.generated.h"

class AIJPArena;
class AIJPBonusTarget;

/**
 * The drifting blocks on an arena (the era's FIJPDriftingBlocks, the 16-bit twist): plain obstacles
 * spread across mid-court that glide up and down, turning back at the walls. A block waits rather
 * than move into a ball, so a ball is never trapped inside one. Placed afresh each match (Reset),
 * and removed in eras without them.
 */
UCLASS(ClassGroup = (IJPong))
class IJPONG_API UIJPDriftingBlockComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UIJPDriftingBlockComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Take them all away; the next tick places them again if the era has them. */
	void Reset();

	int32 GetNumBlocks() const { return Blocks.Num(); }
	const TArray<TObjectPtr<AIJPBonusTarget>>& GetBlocks() const { return Blocks; }

private:
	AIJPArena* GetArena() const;
	void Place();

	UPROPERTY(Transient)
	TArray<TObjectPtr<AIJPBonusTarget>> Blocks;

	/** +1 up, -1 down, one per block. */
	TArray<float> Directions;
};
