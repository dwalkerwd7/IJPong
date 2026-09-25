// It's Just Pong

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "IJPEraSubsystem.generated.h"

class UIJPEra;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FIJPEraChangedSignature, const UIJPEra*, NewEra);

/**
 * Which era the game is in right now. Lives on the game instance, so it survives level loads
 * (a run crossing maps, menus to gameplay). Anything that shows the era reads GetEra() when it
 * starts and listens to OnEraChanged, so an era switch mid-run upgrades everything on screen live.
 */
UCLASS(Config = Game)
class IJPONG_API UIJPEraSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/** The subsystem for WorldContext's game instance; null outside a game (e.g. the editor world). */
	static UIJPEraSubsystem* Get(const UObject* WorldContext);

	/** The current era of WorldContext's game, or null if there is none. */
	static const UIJPEra* GetCurrentEra(const UObject* WorldContext);

	/** Null only if no eras are configured and none was set. */
	UFUNCTION(BlueprintPure, Category = "Era")
	const UIJPEra* GetEra() const { return CurrentEra; }

	/** Make NewEra current and tell everyone. It doesn't have to be one of the configured eras. */
	UFUNCTION(BlueprintCallable, Category = "Era")
	void SetEra(const UIJPEra* NewEra);

	/** Position of the current era in the configured order, or INDEX_NONE. */
	UFUNCTION(BlueprintPure, Category = "Era")
	int32 GetEraIndex() const;

	/** Make the configured era at Index current (clamped to the list). */
	UFUNCTION(BlueprintCallable, Category = "Era")
	void SetEraIndex(int32 Index);

	UFUNCTION(BlueprintPure, Category = "Era")
	int32 GetNumEras() const { return LoadedEras.Num(); }

	/** The Index-th era in order, or null. */
	const UIJPEra* GetEraAt(int32 Index) const { return LoadedEras.IsValidIndex(Index) ? LoadedEras[Index].Get() : nullptr; }

	UPROPERTY(BlueprintAssignable, Category = "Era")
	FIJPEraChangedSignature OnEraChanged;

protected:
	/** Every era, in history order. The first is where the game starts. Set in DefaultGame.ini. */
	UPROPERTY(Config)
	TArray<TSoftObjectPtr<UIJPEra>> Eras;

private:
	UPROPERTY(Transient)
	TArray<TObjectPtr<const UIJPEra>> LoadedEras;

	UPROPERTY(Transient)
	TObjectPtr<const UIJPEra> CurrentEra;
};
