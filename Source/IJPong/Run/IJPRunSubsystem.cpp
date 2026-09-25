// It's Just Pong

#include "Run/IJPRunSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Meta/IJPMetaSubsystem.h"
#include "Run/IJPActConfig.h"
#include "Run/IJPReward.h"

UIJPRunSubsystem* UIJPRunSubsystem::Get(const UObject* WorldContext)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	return GameInstance ? GameInstance->GetSubsystem<UIJPRunSubsystem>() : nullptr;
}

void UIJPRunSubsystem::StartRun(const UIJPActConfig* InAct, int32 Seed, float StartingHealth)
{
	Act = InAct;
	Random.Initialize(Seed);
	Map = Act ? FIJPRunMap::Generate(*Act, Random) : FIJPRunMap();
	Offer.Reset();
	ShopStock.Reset();
	bInShop = false;
	Loadout = FIJPRunLoadout();
	Visited.Init(false, Map.Nodes.Num());
	CurrentNode = INDEX_NONE;
	bInNode = false;
	MaxHealth = Health = FMath::Max(StartingHealth, 1.f);
	Coins = 0;
	EarnedSkillPoints = 0;
	EarnedBossTokens = 0;
	State = Act ? EIJPRunState::Running : EIJPRunState::None;
	OnRunChanged.Broadcast();
}

TArray<int32> UIJPRunSubsystem::GetReachableNodes() const
{
	if (State != EIJPRunState::Running || bInNode || bInShop || HasOffer())
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
	else if (Map.Nodes[Node].Type == EIJPNodeType::Shop)
	{
		ShopStock = Roll(Act->ShopRewards, Act->ShopChoices);
		bInShop = true;
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
			// Boss tokens are paid on the spot; the run's depth points when it ends (now).
			EarnedBossTokens += Act->BossTokens;
			if (UIJPMetaSubsystem* Meta = GetGameInstance()->GetSubsystem<UIJPMetaSubsystem>())
			{
				Meta->AddCurrency(0, Act->BossTokens);
			}
			// A boss always drops an item (for the acts to come).
			if (!Act->BossItems.IsEmpty())
			{
				Loadout.Item = Act->BossItems[Random.RandHelper(Act->BossItems.Num())];
			}
			State = EIJPRunState::Won;
			PayOut();
		}
		else if (const TArray<TObjectPtr<UIJPReward>>* Pool = Act->GetRewardPool(Type))
		{
			RollOffer(*Pool);
		}
	}
	OnRunChanged.Broadcast();
}

void UIJPRunSubsystem::LoseHealth(float Amount)
{
	if (State != EIJPRunState::Running || Amount <= 0.f)
	{
		return;
	}
	Health = FMath::Max(Health - Amount, 0.f);
	if (Health <= 0.f)
	{
		State = EIJPRunState::Lost;
		PayOut();
	}
	OnRunChanged.Broadcast();
}

void UIJPRunSubsystem::RollOffer(const TArray<TObjectPtr<UIJPReward>>& Pool)
{
	Offer = Roll(Pool, Act->RewardChoices);
}

bool UIJPRunSubsystem::BuyFromShop(int32 Index)
{
	if (!bInShop || !ShopStock.IsValidIndex(Index) || Coins < ShopStock[Index]->Price)
	{
		return false;
	}
	Coins -= ShopStock[Index]->Price;
	ShopStock[Index]->Grant(*this);
	ShopStock.RemoveAt(Index);
	OnRunChanged.Broadcast();
	return true;
}

void UIJPRunSubsystem::LeaveShop()
{
	if (bInShop)
	{
		bInShop = false;
		ShopStock.Reset();
		OnRunChanged.Broadcast();
	}
}

TArray<TObjectPtr<const UIJPReward>> UIJPRunSubsystem::Roll(const TArray<TObjectPtr<UIJPReward>>& Pool, int32 Count)
{
	// A few different rewards at random, leaving out any not worth offering now.
	TArray<const UIJPReward*> Candidates;
	for (const UIJPReward* Reward : Pool)
	{
		if (Reward && Reward->CanOffer(*this))
		{
			Candidates.AddUnique(Reward);
		}
	}
	for (int32 i = Candidates.Num() - 1; i > 0; --i)
	{
		Candidates.Swap(i, Random.RandHelper(i + 1));
	}
	TArray<TObjectPtr<const UIJPReward>> Picked;
	for (int32 i = 0; i < FMath::Min(Candidates.Num(), Count); ++i)
	{
		Picked.Add(Candidates[i]);
	}
	return Picked;
}

void UIJPRunSubsystem::TakeReward(int32 Index)
{
	if (!HasOffer())
	{
		return;
	}
	if (Offer.IsValidIndex(Index))
	{
		Offer[Index]->Grant(*this);
	}
	else
	{
		Coins += Act->SkipCoins;
	}
	Offer.Reset();
	OnRunChanged.Broadcast();
}

int32 UIJPRunSubsystem::GetDepthReached() const
{
	int32 Depth = 0;
	for (int32 i = 0; i < Visited.Num(); ++i)
	{
		if (Visited[i])
		{
			Depth = FMath::Max(Depth, Map.Nodes[i].Row + 1);
		}
	}
	return Depth;
}

void UIJPRunSubsystem::PayOut()
{
	EarnedSkillPoints = GetDepthReached() * Act->SkillPointsPerRow;
	if (UIJPMetaSubsystem* Meta = GetGameInstance()->GetSubsystem<UIJPMetaSubsystem>())
	{
		Meta->AddCurrency(EarnedSkillPoints, 0);
	}
}

void UIJPRunSubsystem::RestoreHealth(float Amount)
{
	if (State == EIJPRunState::Running)
	{
		Heal(Amount);
		OnRunChanged.Broadcast();
	}
}

void UIJPRunSubsystem::AddMaxHealth(float Amount)
{
	MaxHealth += FMath::Max(Amount, 0.f);
	Heal(Amount);
}

void UIJPRunSubsystem::Heal(float Amount)
{
	Health = FMath::Min(Health + FMath::Max(Amount, 0.f), MaxHealth);
}
