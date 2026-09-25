// It's Just Pong

#include "Run/IJPRunMap.h"
#include "Run/IJPActConfig.h"

namespace
{
	FIJPMapNode MakeNode(EIJPNodeType Type, int32 Row, int32 Lane)
	{
		FIJPMapNode Node;
		Node.Type = Type;
		Node.Row = Row;
		Node.Lane = Lane;
		return Node;
	}
}

TArray<int32> FIJPRunMap::GetStartNodes() const
{
	TArray<int32> Start;
	for (int32 i = 0; i < Nodes.Num(); ++i)
	{
		if (Nodes[i].Row == 0)
		{
			Start.Add(i);
		}
	}
	Start.Sort([this](int32 A, int32 B) { return Nodes[A].Lane < Nodes[B].Lane; });
	return Start;
}

FIJPRunMap FIJPRunMap::Generate(const UIJPActConfig& Act, FRandomStream& Random)
{
	FIJPRunMap Map;
	Map.Rows = FMath::Max(Act.Rows, 3);
	Map.Lanes = FMath::Max(Act.Lanes, 1);
	const int32 BossRow = Map.Rows - 1;
	const int32 RestRow = BossRow - 1;

	// The boss sits alone in the bottom row, in the middle.
	Map.BossIndex = Map.Nodes.Add(MakeNode(EIJPNodeType::Boss, BossRow, (Map.Lanes - 1) / 2));

	TMap<FIntPoint, int32> NodeAt; // (Row, Lane) -> index
	auto GetOrAdd = [&](int32 Row, int32 Lane)
	{
		if (const int32* Found = NodeAt.Find(FIntPoint(Row, Lane)))
		{
			return *Found;
		}
		const int32 Index = Map.Nodes.Add(MakeNode(EIJPNodeType::Match, Row, Lane));
		NodeAt.Add(FIntPoint(Row, Lane), Index);
		return Index;
	};
	auto Link = [&](int32 From, int32 To) { Map.Nodes[From].Next.AddUnique(To); };

	// Would a step from FromLane (row R) to ToLane (row R+1) cross a path already drawn between those rows?
	auto Crosses = [&](int32 Row, int32 FromLane, int32 ToLane)
	{
		for (const FIJPMapNode& Node : Map.Nodes)
		{
			if (Node.Row != Row)
			{
				continue;
			}
			for (const int32 NextIndex : Node.Next)
			{
				const int32 OtherTo = Map.Nodes[NextIndex].Lane;
				if ((Node.Lane < FromLane && OtherTo > ToLane) || (Node.Lane > FromLane && OtherTo < ToLane))
				{
					return true;
				}
			}
		}
		return false;
	};

	// Walk each path down from the top: one lane left, straight, or one lane right per row.
	int32 FirstStartLane = INDEX_NONE;
	for (int32 Path = 0; Path < FMath::Max(Act.Paths, 1); ++Path)
	{
		int32 Lane = Random.RandHelper(Map.Lanes);
		if (Path == 1 && Map.Lanes > 1)
		{
			// The first two paths start apart, so the very first choice is a real one.
			while (Lane == FirstStartLane)
			{
				Lane = Random.RandHelper(Map.Lanes);
			}
		}
		FirstStartLane = Path == 0 ? Lane : FirstStartLane;

		int32 Previous = GetOrAdd(0, Lane);
		for (int32 Row = 1; Row < BossRow; ++Row)
		{
			TArray<int32> Options;
			for (int32 Step = -1; Step <= 1; ++Step)
			{
				if (Lane + Step >= 0 && Lane + Step < Map.Lanes)
				{
					Options.Add(Lane + Step);
				}
			}
			// Shuffle, then take the first step that doesn't cross an existing path (straight down never can).
			for (int32 i = Options.Num() - 1; i > 0; --i)
			{
				Options.Swap(i, Random.RandHelper(i + 1));
			}
			int32 NextLane = Lane;
			for (const int32 Option : Options)
			{
				if (!Crosses(Row - 1, Lane, Option))
				{
					NextLane = Option;
					break;
				}
			}
			const int32 Current = GetOrAdd(Row, NextLane);
			Link(Previous, Current);
			Previous = Current;
			Lane = NextLane;
		}
		Link(Previous, Map.BossIndex);
	}

	// Types: Match on top, Rest just above the boss, weighted picks in between.
	TArray<TArray<int32>> Predecessors;
	Predecessors.SetNum(Map.Nodes.Num());
	for (int32 i = 0; i < Map.Nodes.Num(); ++i)
	{
		for (const int32 NextIndex : Map.Nodes[i].Next)
		{
			Predecessors[NextIndex].Add(i);
		}
	}
	for (int32 Row = 0; Row < BossRow; ++Row)
	{
		for (int32 i = 0; i < Map.Nodes.Num(); ++i)
		{
			FIJPMapNode& Node = Map.Nodes[i];
			if (Node.Row != Row)
			{
				continue;
			}
			if (Row == 0 || Row == RestRow)
			{
				Node.Type = Row == 0 ? EIJPNodeType::Match : EIJPNodeType::Rest;
				continue;
			}

			// No rest straight after a rest, nor right before the guaranteed one; no elites too early.
			bool bRestAllowed = Row != RestRow - 1;
			for (const int32 Before : Predecessors[i])
			{
				bRestAllowed &= Map.Nodes[Before].Type != EIJPNodeType::Rest;
			}
			const float Match = Act.MatchWeight;
			const float Elite = Row >= Act.EliteFromRow ? Act.EliteWeight : 0.f;
			const float Rest = bRestAllowed ? Act.RestWeight : 0.f;
			const float Total = Match + Elite + Rest;
			const float Roll = Random.FRand() * Total;
			Node.Type = Total <= 0.f || Roll < Match ? EIJPNodeType::Match
				: Roll < Match + Elite ? EIJPNodeType::Elite
				: EIJPNodeType::Rest;
		}
	}

	for (FIJPMapNode& Node : Map.Nodes)
	{
		Node.Next.Sort([&Map](int32 A, int32 B) { return Map.Nodes[A].Lane < Map.Nodes[B].Lane; });
	}
	return Map;
}
