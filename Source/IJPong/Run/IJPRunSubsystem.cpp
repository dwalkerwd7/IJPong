// It's Just Pong

#include "Run/IJPRunSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Run/IJPActConfig.h"

UIJPRunSubsystem* UIJPRunSubsystem::Get(const UObject* WorldContext)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	return GameInstance ? GameInstance->GetSubsystem<UIJPRunSubsystem>() : nullptr;
}

void UIJPRunSubsystem::StartRun(const UIJPActConfig* InAct, int32 Seed, int32 StartingHealth)
{
	Act = InAct;
	FRandomStream Random(Seed);
	Map = Act ? FIJPRunMap::Generate(*Act, Random) : FIJPRunMap();
	Visited.Init(false, Map.Nodes.Num());
	CurrentNode = INDEX_NONE;
	bInNode = false;
	MaxHealth = Health = FMath::Max(StartingHealth, 1);
	Coins = 0;
	State = Act ? EIJPRunState::Running : EIJPRunState::None;
	OnRunChanged.Broadcast();
}

TArray<int32> UIJPRunSubsystem::GetReachableNodes() const
{
	if (State != EIJPRunState::Running || bInNode)
	{
		return {};
	}
	return CurrentNode == INDEX_NONE ? Map.GetStartNodes() : Map.Nodes[CurrentNode].Next;
}

bool UIJPRunSubsystem::EnterNode(int32 Node)
{
	if (!GetReachableNodes().Contains(Node))
	{
		return false;
	}

	CurrentNode = Node;
	Visited[Node] = true;
	if (Map.Nodes[Node].Type == EIJPNodeType::Rest)
	{
		Heal(Act->RestHeal);
	}
	else
	{
		bInNode = true;
	}
	OnRunChanged.Broadcast();
	return true;
}

void UIJPRunSubsystem::CompleteNode(bool bWon)
{
	if (!bInNode)
	{
		return;
	}
	bInNode = false;

	const EIJPNodeType Type = Map.Nodes[CurrentNode].Type;
	if (bWon && State == EIJPRunState::Running)
	{
		if (const FIJPEncounter* Encounter = Act->GetEncounter(Type))
		{
			Coins += Encounter->Coins;
		}
		if (Type == EIJPNodeType::Boss)
		{
			State = EIJPRunState::Won;
		}
	}
	OnRunChanged.Broadcast();
}

void UIJPRunSubsystem::LoseHealth(int32 Amount)
{
	if (State != EIJPRunState::Running || Amount <= 0)
	{
		return;
	}
	Health = FMath::Max(Health - Amount, 0);
	if (Health == 0)
	{
		State = EIJPRunState::Lost;
	}
	OnRunChanged.Broadcast();
}

void UIJPRunSubsystem::Heal(int32 Amount)
{
	Health = FMath::Min(Health + FMath::Max(Amount, 0), MaxHealth);
}
