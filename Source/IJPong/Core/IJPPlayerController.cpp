// It's Just Pong

#include "Core/IJPPlayerController.h"
#include "Core/IJPInputSettings.h"
#include "Core/IJPTypes.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Gameplay/IJPArena.h"
#include "Gameplay/IJPPaddle.h"
#include "InputAction.h"
#include "InputMappingContext.h"

AIJPPlayerController::AIJPPlayerController()
{
	// By default possessing a pawn makes it the view target. We want the arena camera instead.
	bAutoManageActiveCameraTarget = false;
}

void AIJPPlayerController::BeginPlay()
{
	Super::BeginPlay();

	UEnhancedInputLocalPlayerSubsystem* InputSubsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer());
	if (!InputSubsystem)
	{
		return;
	}

	if (UInputMappingContext* Context = GetDefault<UIJPInputSettings>()->PongMappingContext.LoadSynchronous())
	{
		InputSubsystem->AddMappingContext(Context, 0);
	}
	else
	{
		UE_LOG(LogIJPong, Warning, TEXT("IJPong Input settings have no PongMappingContext."));
	}
}

void AIJPPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(InputComponent);
	UInputAction* MoveAction = GetDefault<UIJPInputSettings>()->MovePaddleAction.LoadSynchronous();
	if (EnhancedInput && MoveAction)
	{
		// Triggered fires every frame the axis is non-zero; the paddle consumes input per tick.
		EnhancedInput->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AIJPPlayerController::HandleMove);
	}
	else
	{
		UE_LOG(LogIJPong, Warning, TEXT("Can't bind paddle movement: missing Enhanced Input component or MovePaddleAction."));
	}
}

void AIJPPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (const AIJPPaddle* Paddle = Cast<AIJPPaddle>(InPawn))
	{
		if (AIJPArena* Arena = Paddle->GetArena())
		{
			SetViewTarget(Arena);
		}
	}
}

void AIJPPlayerController::HandleMove(const FInputActionValue& Value)
{
	if (AIJPPaddle* Paddle = GetPawn<AIJPPaddle>())
	{
		Paddle->AddMoveInput(Value.Get<float>());
	}
}
