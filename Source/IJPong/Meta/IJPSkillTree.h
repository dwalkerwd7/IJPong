// It's Just Pong

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "IJPSkillTree.generated.h"

/** What owning a skill-tree node does, in every run from then on. */
UENUM(BlueprintType)
enum class EIJPTreeEffect : uint8
{
	/** Amount = fraction added to the paddle's length. */
	PaddleLength,
	/** Amount = fraction added to the paddle's top speed. */
	PaddleSpeed,
	/** Amount = fraction cut from the class skill's cooldown. */
	SkillCooldown,
	/** Amount = degrees added to how steeply the paddle can return the ball. */
	ReturnAngle,
	/** Amount added to the class skill's named upgrade (UpgradeName); each skill reads the ones it knows. */
	SkillUpgrade
};

USTRUCT(BlueprintType)
struct FIJPSkillNode
{
	GENERATED_BODY()

	/** Stable name within the tree; what the save file remembers. Don't rename once players own it. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Node")
	FName Id;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Node")
	FText DisplayName;

	/** Shown under the tree when the node is picked. Keep it to a line or two. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Node", meta = (MultiLine = "true"))
	FText Description;

	/** Which of the tree's branches it hangs in, 0 = left. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Node", meta = (ClampMin = "0", ClampMax = "2"))
	int32 Branch = 0;

	/** Id of the node that must be owned first. None = hangs from the root (the class skill). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Node")
	FName Parent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Node|Cost", meta = (ClampMin = "0"))
	int32 SkillPoints = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Node|Cost", meta = (ClampMin = "0"))
	int32 BossTokens = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Node|Effect")
	EIJPTreeEffect Effect = EIJPTreeEffect::PaddleLength;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Node|Effect")
	float Amount = 0.1f;

	/** For SkillUpgrade: which upgrade (e.g. Smash reads Power, Curve, ExtraHits, Split). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Node|Effect")
	FName UpgradeName;

	/** A keystone (changes how the skill works) rather than a stat step; drawn differently. */
	bool IsKeystone() const { return BossTokens > 0; }
};

/** Everything the owned nodes of one tree add up to. */
USTRUCT(BlueprintType)
struct FIJPTreeBonuses
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Tree")
	float PaddleLength = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Tree")
	float PaddleSpeed = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Tree")
	float SkillCooldownCut = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Tree")
	float ReturnAngle = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Tree")
	TMap<FName, float> SkillUpgrades;
};

/**
 * A class's skill tree: three branches hanging from the class skill, each a chain of stat nodes
 * (paid in skill points) ending in keystones that change how the skill works (paid in boss
 * tokens). A node can be bought once its parent is owned. What's owned is kept by
 * UIJPMetaSubsystem and applies to every run with that class.
 */
UCLASS(BlueprintType)
class IJPONG_API UIJPSkillTree : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tree")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tree")
	TArray<FIJPSkillNode> Nodes;

	/** Index of the node with Id, or INDEX_NONE. */
	int32 FindNode(FName Id) const
	{
		return Nodes.IndexOfByPredicate([Id](const FIJPSkillNode& Node) { return Node.Id == Id; });
	}

	/** How far down its branch a node sits: 0 = hangs from the root. */
	int32 GetDepth(int32 Index) const
	{
		int32 Depth = 0;
		for (int32 Parent = FindNode(Nodes[Index].Parent); Parent != INDEX_NONE && Depth < Nodes.Num(); Parent = FindNode(Nodes[Parent].Parent))
		{
			++Depth;
		}
		return Depth;
	}
};
