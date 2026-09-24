// It's Just Pong

#pragma once

#include "CoreMinimal.h"
#include "Core/IJPPlayerController.h"
#include "IJPTestPlayerController.generated.h"

class AIJPTestGameMode;

/** The normal player controller plus the test mode's debug keys (reset score, serve now, AI vs AI, opponent skill). */
UCLASS()
class IJPONG_API AIJPTestPlayerController : public AIJPPlayerController
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

private:
	AIJPTestGameMode* GetTestGameMode() const;
	void HandleResetScore();
	void HandleServe();
	void HandleToggleAI();
	void HandleSkillDown();
	void HandleSkillUp();
};
