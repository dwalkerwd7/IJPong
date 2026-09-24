// It's Just Pong

#include "Core/IJPTestPlayerController.h"
#include "Core/IJPInputSettings.h"
#include "Core/IJPTestGameMode.h"
#include "Core/IJPTypes.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/World.h"
#include "InputAction.h"
#include "InputMappingContext.h"

void AIJPTestPlayerController::BeginPlay()
{
	Super::BeginPlay();

	UEnhancedInputLocalPlayerSubsystem* InputSubsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer());
	UInputMappingContext* Context = GetDefault<UIJPInputSettings>()->DebugMappingContext.LoadSynchronous();
	if (InputSubsystem && Context)
	{
		// Above the gameplay context, in case a debug key ever overlaps a gameplay one.
		InputSubsystem->AddMappingContext(Context, 1);
	}
}

void AIJPTestPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(InputComponent);
	if (!EnhancedInput)
	{
		return;
	}

	const UIJPInputSettings* Settings = GetDefault<UIJPInputSettings>();
	auto Bind = [&](const TSoftObjectPtr<UInputAction>& Action, void (AIJPTestPlayerController::*Handler)())
	{
		if (UInputAction* Loaded = Action.LoadSynchronous())
		{
			EnhancedInput->BindAction(Loaded, ETriggerEvent::Started, this, Handler);
		}
		else
		{
			UE_LOG(LogIJPong, Warning, TEXT("IJPong Input settings are missing a debug action; that test key won't work."));
		}
	};
	Bind(Settings->DebugResetScoreAction, &AIJPTestPlayerController::HandleResetScore);
	Bind(Settings->DebugServeAction, &AIJPTestPlayerController::HandleServe);
	Bind(Settings->DebugToggleAIAction, &AIJPTestPlayerController::HandleToggleAI);
	Bind(Settings->DebugSkillDownAction, &AIJPTestPlayerController::HandleSkillDown);
	Bind(Settings->DebugSkillUpAction, &AIJPTestPlayerController::HandleSkillUp);
}

AIJPTestGameMode* AIJPTestPlayerController::GetTestGameMode() const
{
	return GetWorld()->GetAuthGameMode<AIJPTestGameMode>();
}

void AIJPTestPlayerController::HandleResetScore()
{
	if (AIJPTestGameMode* GameMode = GetTestGameMode())
	{
		GameMode->ResetScore();
	}
}

void AIJPTestPlayerController::HandleServe()
{
	if (AIJPTestGameMode* GameMode = GetTestGameMode())
	{
		GameMode->ServeNow();
	}
}

void AIJPTestPlayerController::HandleToggleAI()
{
	if (AIJPTestGameMode* GameMode = GetTestGameMode())
	{
		GameMode->SetPlayerSideAI(!GameMode->IsPlayerSideAI());
	}
}

void AIJPTestPlayerController::HandleSkillDown()
{
	if (AIJPTestGameMode* GameMode = GetTestGameMode())
	{
		GameMode->AdjustOpponentSkill(-0.1f);
	}
}

void AIJPTestPlayerController::HandleSkillUp()
{
	if (AIJPTestGameMode* GameMode = GetTestGameMode())
	{
		GameMode->AdjustOpponentSkill(0.1f);
	}
}
