// It's Just Pong

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "IJPPlayerController.generated.h"

struct FInputActionValue;

/**
 * Adds the Pong mapping context, forwards the move axis and ability buttons to the possessed paddle,
 * and keeps the view on the paddle's arena camera instead of the pawn.
 */
UCLASS()
class IJPONG_API AIJPPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AIJPPlayerController();

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void OnPossess(APawn* InPawn) override;

private:
	void HandleMove(const FInputActionValue& Value);
	void HandleClassSkill();
	void HandleRunAbility();
};
