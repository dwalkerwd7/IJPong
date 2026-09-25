// It's Just Pong

#include "Core/IJPPlayerController.h"
#include "Abilities/IJPAbilityComponent.h"
#include "Core/IJPGameModeBase.h"
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
	const UIJPInputSettings* Settings = GetDefault<UIJPInputSettings>();
	UInputAction* MoveAction = Settings->MovePaddleAction.LoadSynchronous();
	if (EnhancedInput && MoveAction)
	{
		// Triggered fires every frame the axis is non-zero; the paddle consumes input per tick.
		EnhancedInput->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AIJPPlayerController::HandleMove);
	}
	else
	{
		UE_LOG(LogIJPong, Warning, TEXT("Can't bind paddle movement: missing Enhanced Input component or MovePaddleAction."));
	}

	if (!EnhancedInput)
	{
		return;
	}
	if (UInputAction* ClassSkill = Settings->ClassSkillAction.LoadSynchronous())
	{
		EnhancedInput->BindAction(ClassSkill, ETriggerEvent::Started, this, &AIJPPlayerController::HandleClassSkill);
		EnhancedInput->BindAction(ClassSkill, ETriggerEvent::Completed, this, &AIJPPlayerController::HandleClassSkillReleased);
	}
	if (UInputAction* Aim = Settings->AimAction.LoadSynchronous())
	{
		EnhancedInput->BindAction(Aim, ETriggerEvent::Triggered, this, &AIJPPlayerController::HandleAim);
	}
	if (UInputAction* AimKeys = Settings->AimKeysAction.LoadSynchronous())
	{
		// Triggered every frame a key is held: the aim turns at a steady rate.
		EnhancedInput->BindAction(AimKeys, ETriggerEvent::Triggered, this, &AIJPPlayerController::HandleAimKeys);
	}
	if (UInputAction* UIStep = Settings->UIStepAction.LoadSynchronous())
	{
		// Started: one step per press, not one per frame held.
		EnhancedInput->BindAction(UIStep, ETriggerEvent::Started, this, &AIJPPlayerController::HandleUIStep);
	}
	if (UInputAction* Spell = Settings->SpellAction.LoadSynchronous())
	{
		EnhancedInput->BindAction(Spell, ETriggerEvent::Started, this, &AIJPPlayerController::HandleSpell);
	}
	if (UInputAction* Item = Settings->ItemAction.LoadSynchronous())
	{
		EnhancedInput->BindAction(Item, ETriggerEvent::Started, this, &AIJPPlayerController::HandleItem);
	}
	if (UInputAction* RunAbility = Settings->RunAbilityAction.LoadSynchronous())
	{
		EnhancedInput->BindAction(RunAbility, ETriggerEvent::Started, this, &AIJPPlayerController::HandleRunAbility);
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

void AIJPPlayerController::HandleUIStep(const FInputActionValue& Value)
{
	const float Axis = Value.Get<float>();
	if (AIJPGameModeBase* GameMode = GetWorld()->GetAuthGameMode<AIJPGameModeBase>(); GameMode && Axis != 0.f)
	{
		GameMode->HandleUIStep(Axis > 0.f ? 1 : -1);
	}
}

void AIJPPlayerController::HandleClassSkill()
{
	// On a menu or the map, the button confirms instead.
	if (AIJPGameModeBase* GameMode = GetWorld()->GetAuthGameMode<AIJPGameModeBase>(); GameMode && GameMode->HandleUIConfirm())
	{
		return;
	}
	if (AIJPPaddle* Paddle = GetPawn<AIJPPaddle>())
	{
		Paddle->GetAbilities()->TryActivate(EIJPAbilitySlot::ClassSkill);
	}
}

void AIJPPlayerController::HandleClassSkillReleased()
{
	if (AIJPPaddle* Paddle = GetPawn<AIJPPaddle>())
	{
		Paddle->GetAbilities()->Release(EIJPAbilitySlot::ClassSkill);
	}
}

void AIJPPlayerController::HandleAim(const FInputActionValue& Value)
{
	// A stick points where to aim; ignore it near the centre so letting go doesn't snap the aim.
	const FVector2D Stick = Value.Get<FVector2D>();
	AIJPPaddle* Paddle = GetPawn<AIJPPaddle>();
	if (Paddle && Stick.SizeSquared() > 0.3f * 0.3f)
	{
		Paddle->SetAimDirection(Stick);
	}
}

void AIJPPlayerController::HandleAimKeys(const FInputActionValue& Value)
{
	if (AIJPPaddle* Paddle = GetPawn<AIJPPaddle>())
	{
		Paddle->AddAimAngle(Value.Get<float>() * GetDefault<UIJPInputSettings>()->KeyAimSpeed * GetWorld()->GetDeltaSeconds());
	}
}

void AIJPPlayerController::HandleSpell()
{
	if (AIJPPaddle* Paddle = GetPawn<AIJPPaddle>())
	{
		Paddle->GetAbilities()->TryActivate(EIJPAbilitySlot::Spell);
	}
}

void AIJPPlayerController::HandleItem()
{
	if (AIJPPaddle* Paddle = GetPawn<AIJPPaddle>())
	{
		Paddle->GetAbilities()->TryActivate(EIJPAbilitySlot::Item);
	}
}

void AIJPPlayerController::HandleRunAbility()
{
	if (AIJPPaddle* Paddle = GetPawn<AIJPPaddle>())
	{
		Paddle->GetAbilities()->TryActivate(EIJPAbilitySlot::RunAbility);
	}
}

void AIJPPlayerController::HandleMove(const FInputActionValue& Value)
{
	if (AIJPPaddle* Paddle = GetPawn<AIJPPaddle>())
	{
		Paddle->AddMoveInput(Value.Get<float>());
	}
}
