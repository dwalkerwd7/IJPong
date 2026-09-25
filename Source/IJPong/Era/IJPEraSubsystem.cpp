// It's Just Pong

#include "Era/IJPEraSubsystem.h"
#include "Core/IJPTypes.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Era/IJPEra.h"

void UIJPEraSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	for (const TSoftObjectPtr<UIJPEra>& Era : Eras)
	{
		if (const UIJPEra* Loaded = Era.LoadSynchronous())
		{
			LoadedEras.Add(Loaded);
		}
		else
		{
			UE_LOG(LogIJPong, Warning, TEXT("Era '%s' in the IJPEraSubsystem config didn't load; skipped."), *Era.ToString());
		}
	}

	// No broadcast: nothing has started listening yet, and listeners read GetEra() when they start.
	CurrentEra = LoadedEras.IsEmpty() ? nullptr : LoadedEras[0].Get();
}

UIJPEraSubsystem* UIJPEraSubsystem::Get(const UObject* WorldContext)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	return GameInstance ? GameInstance->GetSubsystem<UIJPEraSubsystem>() : nullptr;
}

const UIJPEra* UIJPEraSubsystem::GetCurrentEra(const UObject* WorldContext)
{
	const UIJPEraSubsystem* Subsystem = Get(WorldContext);
	return Subsystem ? Subsystem->GetEra() : nullptr;
}

void UIJPEraSubsystem::SetEra(const UIJPEra* NewEra)
{
	if (NewEra == CurrentEra)
	{
		return;
	}
	CurrentEra = NewEra;
	OnEraChanged.Broadcast(CurrentEra);
}

int32 UIJPEraSubsystem::GetEraIndex() const
{
	return LoadedEras.IndexOfByKey(CurrentEra);
}

void UIJPEraSubsystem::SetEraIndex(int32 Index)
{
	if (!LoadedEras.IsEmpty())
	{
		SetEra(LoadedEras[FMath::Clamp(Index, 0, LoadedEras.Num() - 1)]);
	}
}
