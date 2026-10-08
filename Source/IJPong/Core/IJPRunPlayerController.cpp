// It's Just Pong

#include "Core/IJPRunPlayerController.h"
#include "Core/IJPInputSettings.h"
#include "Core/IJPRunGameMode.h"
#include "Core/IJPTypes.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/World.h"
#include "InputAction.h"
#include "InputMappingContext.h"

void AIJPRunPlayerController::BeginPlay()
{
	Super::BeginPlay();

#if !UE_BUILD_SHIPPING
	UEnhancedInputLocalPlayerSubsystem* InputSubsystem = GetLocalPlayer() ? ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()) : nullptr;
	UInputMappingContext* Context = GetDefault<UIJPInputSettings>()->CheatMappingContext.LoadSynchronous();
	if (InputSubsystem && Context)
	{
		InputSubsystem->AddMappingContext(Context, 1);
	}
#endif
}

void AIJPRunPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

#if !UE_BUILD_SHIPPING
	UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(InputComponent);
	if (!EnhancedInput)
	{
		return;
	}
	const UIJPInputSettings* Settings = GetDefault<UIJPInputSettings>();
	auto Bind = [&](const TSoftObjectPtr<UInputAction>& Action, void (AIJPRunPlayerController::*Handler)())
	{
		if (UInputAction* Loaded = Action.LoadSynchronous())
		{
			EnhancedInput->BindAction(Loaded, ETriggerEvent::Started, this, Handler);
		}
		else
		{
			UE_LOG(LogIJPong, Warning, TEXT("IJPong Input settings are missing a cheat action; that cheat key won't work."));
		}
	};
	Bind(Settings->CheatWinAction, &AIJPRunPlayerController::HandleWin);
	Bind(Settings->CheatLoseAction, &AIJPRunPlayerController::HandleLose);
	Bind(Settings->CheatHealAction, &AIJPRunPlayerController::HandleHeal);
	Bind(Settings->CheatCoinsAction, &AIJPRunPlayerController::HandleCoins);
	Bind(Settings->CheatMetaAction, &AIJPRunPlayerController::HandleMeta);
	Bind(Settings->CheatUnlockEraAction, &AIJPRunPlayerController::HandleUnlockEra);
#endif
}

AIJPRunGameMode* AIJPRunPlayerController::GetRunGameMode() const
{
	return GetWorld()->GetAuthGameMode<AIJPRunGameMode>();
}

void AIJPRunPlayerController::HandleWin()
{
	if (AIJPRunGameMode* GameMode = GetRunGameMode())
	{
		GameMode->CheatEndMatch(true);
	}
}

void AIJPRunPlayerController::HandleLose()
{
	if (AIJPRunGameMode* GameMode = GetRunGameMode())
	{
		GameMode->CheatEndMatch(false);
	}
}

void AIJPRunPlayerController::HandleHeal()
{
	if (AIJPRunGameMode* GameMode = GetRunGameMode())
	{
		GameMode->CheatHeal();
	}
}

void AIJPRunPlayerController::HandleCoins()
{
	if (AIJPRunGameMode* GameMode = GetRunGameMode())
	{
		GameMode->CheatCoins(50);
	}
}

void AIJPRunPlayerController::HandleMeta()
{
	if (AIJPRunGameMode* GameMode = GetRunGameMode())
	{
		GameMode->CheatMeta(10, 3);
	}
}

void AIJPRunPlayerController::HandleUnlockEra()
{
	if (AIJPRunGameMode* GameMode = GetRunGameMode())
	{
		GameMode->CheatUnlockNextEra();
	}
}
