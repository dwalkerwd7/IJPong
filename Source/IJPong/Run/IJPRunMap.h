// It's Just Pong

#pragma once

#include "CoreMinimal.h"
#include "IJPRunMap.generated.h"

class UIJPActConfig;

/** What happens at a map node. */
UENUM(BlueprintType)
enum class EIJPNodeType : uint8
{
	Match,
	/** A tougher rival for a bigger reward. */
	Elite,
	/** Recover health. */
	Rest,
	/** Not built yet: never generated. */
	Shop,
	/** Not built yet: never generated. */
	Event,
	/** Ends the act. */
	Boss
};

USTRUCT(BlueprintType)
struct FIJPMapNode
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Map")
	EIJPNodeType Type = EIJPNodeType::Match;

	/** Step down the map: 0 = the top row (the first choice); the boss is alone in the bottom row. */
	UPROPERTY(BlueprintReadOnly, Category = "Map")
	int32 Row = 0;

	/** Which of the side-by-side lanes it sits in, 0 = leftmost .. Lanes-1. */
	UPROPERTY(BlueprintReadOnly, Category = "Map")
	int32 Lane = 0;

	/** Nodes this one leads to (indices into FIJPRunMap::Nodes), in lane order. */
	UPROPERTY(BlueprintReadOnly, Category = "Map")
	TArray<int32> Next;
};

/**
 * One act's branching map, Slay the Spire style, read top to bottom: rows of nodes from a first
 * choice of fights down to a single boss, joined by paths that never cross.
 */
USTRUCT(BlueprintType)
struct FIJPRunMap
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Map")
	TArray<FIJPMapNode> Nodes;

	UPROPERTY(BlueprintReadOnly, Category = "Map")
	int32 Rows = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Map")
	int32 Lanes = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Map")
	int32 BossIndex = INDEX_NONE;

	/** Nodes in the top row: where every path starts. */
	TArray<int32> GetStartNodes() const;

	/**
	 * Build a map from an act's shape and weights. The same act and stream state always give the
	 * same map, so a run is reproducible from its seed.
	 */
	static FIJPRunMap Generate(const UIJPActConfig& Act, FRandomStream& Random);
};
