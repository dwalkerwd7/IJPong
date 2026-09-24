// It's Just Pong

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "IJPInputSettings.generated.h"

class UInputAction;
class UInputMappingContext;

/**
 * Project Settings > Game > IJPong Input.
 * Points C++ at the Enhanced Input assets, so the player controller needs no Blueprint subclass.
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "IJPong Input"))
class IJPONG_API UIJPInputSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UPROPERTY(Config, EditAnywhere, Category = "Input")
	TSoftObjectPtr<UInputMappingContext> PongMappingContext;

	/** Axis1D: +1 moves the paddle up the screen, -1 down. */
	UPROPERTY(Config, EditAnywhere, Category = "Input")
	TSoftObjectPtr<UInputAction> MovePaddleAction;

	/** Only added in the test game mode. */
	UPROPERTY(Config, EditAnywhere, Category = "Debug")
	TSoftObjectPtr<UInputMappingContext> DebugMappingContext;

	UPROPERTY(Config, EditAnywhere, Category = "Debug")
	TSoftObjectPtr<UInputAction> DebugResetScoreAction;

	UPROPERTY(Config, EditAnywhere, Category = "Debug")
	TSoftObjectPtr<UInputAction> DebugServeAction;

	UPROPERTY(Config, EditAnywhere, Category = "Debug")
	TSoftObjectPtr<UInputAction> DebugToggleAIAction;

	UPROPERTY(Config, EditAnywhere, Category = "Debug")
	TSoftObjectPtr<UInputAction> DebugSkillDownAction;

	UPROPERTY(Config, EditAnywhere, Category = "Debug")
	TSoftObjectPtr<UInputAction> DebugSkillUpAction;
};
