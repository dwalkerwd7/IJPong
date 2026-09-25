// It's Just Pong

#include "Meta/IJPMetaSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Meta/IJPMetaSave.h"

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
