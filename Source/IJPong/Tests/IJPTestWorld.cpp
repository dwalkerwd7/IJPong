// It's Just Pong

#include "Tests/IJPTestWorld.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Gameplay/IJPArena.h"

FIJPTestWorld::FIJPTestWorld(const FTransform& ArenaTransform, TFunction<void(AIJPArena&)> SetupArena)
{
	// A standalone game instance creates its own Game world context and world, like a packaged game does.
	GameInstance = NewObject<UGameInstance>(GEngine);
	GameInstance->AddToRoot();
	GameInstance->InitializeStandalone(MakeUniqueObjectName(nullptr, UWorld::StaticClass(), TEXT("IJPTestWorld"), EUniqueObjectNameOptions::GloballyUnique));
	World = GameInstance->GetWorld();
	check(World);
	World->AddToRoot();

	// Same order as UEngine::LoadMap: game mode, level actors, initialise, begin play.
	World->SetGameMode(FURL());
	Arena = World->SpawnActor<AIJPArena>(AIJPArena::StaticClass(), ArenaTransform);
	if (SetupArena)
	{
		SetupArena(*Arena);
	}
	World->InitializeActorsForPlay(FURL());
	World->BeginPlay();
}

FIJPTestWorld::~FIJPTestWorld()
{
	GameInstance->Shutdown();

	World->EndPlay(EEndPlayReason::LevelTransition);
	GEngine->ShutdownWorldNetDriver(World);
	World->DestroyWorld(true);
	GEngine->DestroyWorldContext(World);

	World->RemoveFromRoot();
	GameInstance->RemoveFromRoot();
}

void FIJPTestWorld::Step()
{
	// Tick functions run at most once per engine frame, so each step has to be a new frame.
	++GFrameCounter;
	World->Tick(LEVELTICK_All, FixedStep);
}

void FIJPTestWorld::RunFor(float Seconds, TFunctionRef<void()> BeforeEachStep)
{
	const int32 Steps = FMath::CeilToInt(Seconds / FixedStep);
	for (int32 i = 0; i < Steps; ++i)
	{
		BeforeEachStep();
		Step();
	}
}

#endif
