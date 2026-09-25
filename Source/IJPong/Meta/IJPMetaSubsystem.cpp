// It's Just Pong

#include "Meta/IJPMetaSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Meta/IJPMetaSave.h"
#include "Meta/IJPSkillTree.h"

void UIJPMetaSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	const FString Slot = GetSlot();
	SaveData = UGameplayStatics::DoesSaveGameExist(Slot, 0) ? Cast<UIJPMetaSave>(UGameplayStatics::LoadGameFromSlot(Slot, 0)) : nullptr;
	if (!SaveData)
	{
		SaveData = Cast<UIJPMetaSave>(UGameplayStatics::CreateSaveGameObject(UIJPMetaSave::StaticClass()));
	}
}

UIJPMetaSubsystem* UIJPMetaSubsystem::Get(const UObject* WorldContext)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	return GameInstance ? GameInstance->GetSubsystem<UIJPMetaSubsystem>() : nullptr;
}

int32 UIJPMetaSubsystem::GetSkillPoints() const
{
	return SaveData->SkillPoints;
}

int32 UIJPMetaSubsystem::GetBossTokens() const
{
	return SaveData->BossTokens;
}

void UIJPMetaSubsystem::AddCurrency(int32 SkillPoints, int32 BossTokens)
{
	if (SkillPoints == 0 && BossTokens == 0)
	{
		return;
	}
	SaveData->SkillPoints = FMath::Max(SaveData->SkillPoints + SkillPoints, 0);
	SaveData->BossTokens = FMath::Max(SaveData->BossTokens + BossTokens, 0);
	Save();
	OnMetaChanged.Broadcast();
}

FString UIJPMetaSubsystem::NodeKey(const UIJPSkillTree& Tree, int32 Node)
{
	return Tree.GetName() + TEXT("/") + Tree.Nodes[Node].Id.ToString();
}

bool UIJPMetaSubsystem::IsOwned(const UIJPSkillTree* Tree, int32 Node) const
{
	return Tree && Tree->Nodes.IsValidIndex(Node) && SaveData->OwnedNodes.Contains(NodeKey(*Tree, Node));
}

bool UIJPMetaSubsystem::CanBuy(const UIJPSkillTree* Tree, int32 Node) const
{
	if (!Tree || !Tree->Nodes.IsValidIndex(Node) || IsOwned(Tree, Node))
	{
		return false;
	}
	const FIJPSkillNode& Data = Tree->Nodes[Node];
	const int32 Parent = Tree->FindNode(Data.Parent);
	const bool bParentOwned = Data.Parent.IsNone() || IsOwned(Tree, Parent);
	return bParentOwned && SaveData->SkillPoints >= Data.SkillPoints && SaveData->BossTokens >= Data.BossTokens;
}

bool UIJPMetaSubsystem::Buy(const UIJPSkillTree* Tree, int32 Node)
{
	if (!CanBuy(Tree, Node))
	{
		return false;
	}
	const FIJPSkillNode& Data = Tree->Nodes[Node];
	SaveData->SkillPoints -= Data.SkillPoints;
	SaveData->BossTokens -= Data.BossTokens;
	SaveData->OwnedNodes.Add(NodeKey(*Tree, Node));
	Save();
	OnMetaChanged.Broadcast();
	return true;
}

FIJPTreeBonuses UIJPMetaSubsystem::GetBonuses(const UIJPSkillTree* Tree) const
{
	FIJPTreeBonuses Bonuses;
	if (!Tree)
	{
		return Bonuses;
	}
	for (int32 i = 0; i < Tree->Nodes.Num(); ++i)
	{
		if (!IsOwned(Tree, i))
		{
			continue;
		}
		const FIJPSkillNode& Node = Tree->Nodes[i];
		switch (Node.Effect)
		{
		case EIJPTreeEffect::PaddleLength:  Bonuses.PaddleLength += Node.Amount; break;
		case EIJPTreeEffect::PaddleSpeed:   Bonuses.PaddleSpeed += Node.Amount; break;
		case EIJPTreeEffect::SkillCooldown: Bonuses.SkillCooldownCut += Node.Amount; break;
		case EIJPTreeEffect::ReturnAngle:   Bonuses.ReturnAngle += Node.Amount; break;
		case EIJPTreeEffect::SkillUpgrade:  Bonuses.SkillUpgrades.FindOrAdd(Node.UpgradeName) += Node.Amount; break;
		}
	}
	return Bonuses;
}

int32 UIJPMetaSubsystem::GetErasUnlocked() const
{
	return SaveData ? FMath::Max(SaveData->ErasUnlocked, 1) : 1;
}

bool UIJPMetaSubsystem::UnlockErasUpTo(int32 Count)
{
	if (!SaveData || Count <= GetErasUnlocked())
	{
		return false;
	}
	SaveData->ErasUnlocked = Count;
	Save();
	OnMetaChanged.Broadcast();
	return true;
}

bool UIJPMetaSubsystem::HasSeenEraCard(const FString& EraName) const
{
	return SaveData && SaveData->SeenEraCards.Contains(EraName);
}

void UIJPMetaSubsystem::MarkEraCardSeen(const FString& EraName)
{
	if (SaveData && !SaveData->SeenEraCards.Contains(EraName))
	{
		SaveData->SeenEraCards.Add(EraName);
		Save();
	}
}

bool UIJPMetaSubsystem::IsSpellSlotUnlocked() const
{
	return SaveData && SaveData->bSpellSlotUnlocked;
}

bool UIJPMetaSubsystem::CanUnlockSpellSlot() const
{
	return SaveData && !SaveData->bSpellSlotUnlocked && SaveData->BossTokens >= SpellSlotCost;
}

bool UIJPMetaSubsystem::UnlockSpellSlot()
{
	if (!CanUnlockSpellSlot())
	{
		return false;
	}
	SaveData->BossTokens -= SpellSlotCost;
	SaveData->bSpellSlotUnlocked = true;
	Save();
	OnMetaChanged.Broadcast();
	return true;
}

void UIJPMetaSubsystem::ResetProgress()
{
	SaveData = Cast<UIJPMetaSave>(UGameplayStatics::CreateSaveGameObject(UIJPMetaSave::StaticClass()));
	UGameplayStatics::DeleteGameInSlot(GetSlot(), 0);
	OnMetaChanged.Broadcast();
}

FString UIJPMetaSubsystem::GetSlot() const
{
	return GIsAutomationTesting ? TestSaveSlot : SaveSlot;
}

void UIJPMetaSubsystem::Save() const
{
	UGameplayStatics::SaveGameToSlot(SaveData, GetSlot(), 0);
}
