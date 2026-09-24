// It's Just Pong

#pragma once

#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS

class AIJPArena;
class UGameInstance;
class UWorld;

/**
 * A real game world for integration tests: game instance, the project's default game mode,
 * one arena, and BeginPlay run through the normal StartPlay path. Torn down in the destructor.
 * Ticks at a fixed step so results are deterministic.
 */
class FIJPTestWorld
{
public:
	static constexpr float FixedStep = 1.f / 60.f;

	/** @param SetupArena  Optional: configure the arena after it's spawned but before BeginPlay (e.g. paddle profiles). */
	explicit FIJPTestWorld(const FTransform& ArenaTransform = FTransform::Identity, TFunction<void(AIJPArena&)> SetupArena = nullptr);
	~FIJPTestWorld();

	UWorld* GetWorld() const { return World; }
	AIJPArena* GetArena() const { return Arena; }

	/** Advance one fixed step. */
	void Step();

	/** Advance whole fixed steps covering Seconds, calling BeforeEachStep first on every step (e.g. to feed input). */
	void RunFor(float Seconds, TFunctionRef<void()> BeforeEachStep = [] {});

private:
	UGameInstance* GameInstance = nullptr;
	UWorld* World = nullptr;
	AIJPArena* Arena = nullptr;
};

#endif
