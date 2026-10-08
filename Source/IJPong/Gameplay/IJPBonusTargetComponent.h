// It's Just Pong

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Core/IJPTypes.h"
#include "IJPBonusTargetComponent.generated.h"

class AIJPArena;
class AIJPBall;
class AIJPBonusTarget;

/** A ball returned by Side broke a target worth Coins. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FIJPBonusTargetHitSignature, EIJPSide, Side, int32, Coins);

/**
 * The bonus targets on an arena (the era's FIJPBonusTargets, the Arcade twist). While a ball is in
 * play, a new target appears every SpawnInterval somewhere mid-court, up to MaxTargets. A ball breaks
 * one and bounces off; if a paddle returned that ball, OnTargetHit says whose (the run pays coins).
 * A new match, its end, or an era change clears them.
 */
UCLASS(ClassGroup = (IJPong))
class IJPONG_API UIJPBonusTargetComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UIJPBonusTargetComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Put a target at a plane position now (tests, debugging), whatever the era. */
	AIJPBonusTarget* SpawnTargetAt(const FVector2D& Centre);

	/** Remove every target and restart the spawn countdown. */
	void Clear();

	int32 GetNumTargets() const { return Targets.Num(); }
	const TArray<TObjectPtr<AIJPBonusTarget>>& GetTargets() const { return Targets; }

	/** Called by a target as a ball breaks it. */
	void HandleTargetBroken(AIJPBonusTarget* Target, AIJPBall* Ball);

	/**
	 * A random spot mid-court for something Size across, within CourtFraction of the half-court from
	 * the net, clear of the balls and the targets (and of Avoid). False if none was found.
	 */
	bool PickSpot(float Size, float CourtFraction, FVector2D& OutCentre, const TArray<FVector2D>& Avoid = {}) const;

	UPROPERTY(BlueprintAssignable, Category = "Bonus Targets")
	FIJPBonusTargetHitSignature OnTargetHit;

private:
	AIJPArena* GetArena() const;

	UPROPERTY(Transient)
	TArray<TObjectPtr<AIJPBonusTarget>> Targets;

	float SpawnCountdown = 0.f;
};
