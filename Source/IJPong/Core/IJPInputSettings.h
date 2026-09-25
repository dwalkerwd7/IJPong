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

	/** Axis1D: -1 left / +1 right, for menus and the run map. */
	UPROPERTY(Config, EditAnywhere, Category = "Input")
	TSoftObjectPtr<UInputAction> UIStepAction;

	/** Digital: use the paddle's class skill (also confirms on menus and the run map). */
	UPROPERTY(Config, EditAnywhere, Category = "Input")
	TSoftObjectPtr<UInputAction> ClassSkillAction;

	/** Axis2D: aim with a stick (right stick), a direction on screen. */
	UPROPERTY(Config, EditAnywhere, Category = "Input")
	TSoftObjectPtr<UInputAction> AimAction;

	/** Axis2D: mouse movement, which nudges the aim up and down. */
	UPROPERTY(Config, EditAnywhere, Category = "Input")
	TSoftObjectPtr<UInputAction> AimMouseAction;

	/** Degrees of aim per unit of mouse movement. */
	UPROPERTY(Config, EditAnywhere, Category = "Input", meta = (ClampMin = "0"))
	float MouseAimSensitivity = 0.5f;

	/** Digital: use the paddle's run ability. */
	UPROPERTY(Config, EditAnywhere, Category = "Input")
	TSoftObjectPtr<UInputAction> RunAbilityAction;

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

	UPROPERTY(Config, EditAnywhere, Category = "Debug")
	TSoftObjectPtr<UInputAction> DebugToggleOverlayAction;

	UPROPERTY(Config, EditAnywhere, Category = "Debug")
	TSoftObjectPtr<UInputAction> DebugEraPrevAction;

	UPROPERTY(Config, EditAnywhere, Category = "Debug")
	TSoftObjectPtr<UInputAction> DebugEraNextAction;

	UPROPERTY(Config, EditAnywhere, Category = "Debug")
	TSoftObjectPtr<UInputAction> DebugAddBallAction;

	UPROPERTY(Config, EditAnywhere, Category = "Debug")
	TSoftObjectPtr<UInputAction> DebugCyclePlayerClassAction;

	UPROPERTY(Config, EditAnywhere, Category = "Debug")
	TSoftObjectPtr<UInputAction> DebugCycleRivalAction;
};
