// It's Just Pong

#pragma once

#include "CoreMinimal.h"
#include "Core/IJPPlayerController.h"
#include "IJPRunPlayerController.generated.h"

class AIJPRunGameMode;

/**
 * The normal player controller plus the run's debug cheats (IMC_Cheats; never in shipping builds):
 * K win the match, L lose it, H full heal, J +50 coins, G +10 skill points and +3 boss tokens,
 * U unlock the next era. The cheats themselves live on AIJPRunGameMode.
 */
UCLASS()
class IJPONG_API AIJPRunPlayerController : public AIJPPlayerController
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

private:
	AIJPRunGameMode* GetRunGameMode() const;
	void HandleWin();
	void HandleLose();
	void HandleHeal();
	void HandleCoins();
	void HandleMeta();
	void HandleUnlockEra();
};
