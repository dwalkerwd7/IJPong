// It's Just Pong

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "IJPLightTrailComponent.generated.h"

class AIJPArena;
class AIJPBall;
class UBoxComponent;
class UStaticMeshComponent;

/**
 * Light trails on an arena (the era's FIJPLightTrails, the Grid twist). Every ball in play lays a
 * thin wall behind it, a piece every PieceLength; a piece turns solid after SolidDelay (so a ball
 * never hits what it's laying) and vanishes after Lifetime. Any ball bounces off any solid piece,
 * its own included; paddles pass through. Pieces are pooled components on the arena.
 */
UCLASS(ClassGroup = (IJPong))
class IJPONG_API UIJPLightTrailComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UIJPLightTrailComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Every piece gone, every ball starting afresh. */
	void Reset();

	/** Add the solid pieces running nearly parallel to Motion (within ParallelDeg) to a ball's sweep's ignore list. */
	void AddParallelPieces(const FVector2D& Motion, struct FCollisionQueryParams& Params) const;

	/** Pieces on the court now, and how many of them are solid. */
	int32 GetNumPieces() const;
	int32 GetNumSolidPieces() const;

private:
	struct FPiece
	{
		TObjectPtr<UBoxComponent> Box;
		TObjectPtr<UStaticMeshComponent> Visual;
		FVector2D Direction = FVector2D::ZeroVector;
		float Age = 0.f;
		bool bActive = false;
		bool bSolid = false;
	};

	AIJPArena* GetArena() const;
	void Lay(const FVector2D& From, const FVector2D& To, const struct FIJPLightTrails& Settings);
	FPiece& TakePiece();
	void Retire(FPiece& Piece);

	TArray<FPiece> Pieces;

	/** Where each ball's trail was last anchored. */
	TMap<TWeakObjectPtr<AIJPBall>, FVector2D> Anchors;

	/** Keeps the pooled components alive (FPiece isn't reflected). */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UObject>> Owned;
};
