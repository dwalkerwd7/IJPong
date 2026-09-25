// It's Just Pong

#include "Core/IJPTestGameMode.h"
#include "AI/IJPPaddleAIController.h"
#include "Core/IJPTestHUD.h"
#include "Core/IJPTestPlayerController.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "Era/IJPEra.h"
#include "Era/IJPEraSubsystem.h"
#include "Gameplay/IJPArena.h"
#include "Gameplay/IJPMatchComponent.h"
#include "Gameplay/IJPMatchRules.h"
#include "Gameplay/IJPPaddle.h"

AIJPTestGameMode::AIJPTestGameMode()
{
	PlayerControllerClass = AIJPTestPlayerController::StaticClass();
	HUDClass = AIJPTestHUD::StaticClass();
}

void AIJPTestGameMode::OnArenaReady()
{
	RestartMatch();
}

void AIJPTestGameMode::RestartMatch(const UIJPMatchRules* Rules)
{
	GetMatch()->StartMatch(GetArena(), Rules ? Rules : MatchRules.LoadSynchronous());
}

void AIJPTestGameMode::ServeNow()
{
	if (GetMatch()->IsOver())
	{
		RestartMatch();
	}
	else
	{
		GetMatch()->ServeNow();
	}
}

void AIJPTestGameMode::SetPlayerSideAI(bool bEnable)
{
	AIJPPaddle* Paddle = GetArena() ? GetArena()->GetPaddle(PlayerSide) : nullptr;
	if (!Paddle || bEnable == IsPlayerSideAI())
	{
		return;
	}

	if (bEnable)
	{
		// The player controller keeps the arena camera and its debug keys; it just stops driving the paddle.
		if (AController* Current = Paddle->GetController())
		{
			Current->UnPossess();
		}
		PlayerSideAI = SpawnAIPaddle(PlayerSide);
	}
	else
	{
		PlayerSideAI->UnPossess();
		PlayerSideAI->Destroy();
		PlayerSideAI = nullptr;

		for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
		{
			PossessPlayerPaddle(It->Get());
		}
	}
}

void AIJPTestGameMode::AdjustOpponentSkill(float Delta)
{
	AIJPArena* ArenaPtr = GetArena();
	if (!ArenaPtr)
	{
		return;
	}

	ArenaPtr->SetOpponentSkill(ArenaPtr->GetOpponentSkill() + Delta);
	const float Skill = ArenaPtr->GetOpponentSkill();
	for (TActorIterator<AIJPPaddleAIController> It(GetWorld()); It; ++It)
	{
		It->SetSkill(Skill);
	}

	UE_LOG(LogIJPong, Log, TEXT("Opponent skill: %.2f"), Skill);
	if (GEngine)
	{
		// Same key each time, so repeated presses replace the message instead of stacking.
		GEngine->AddOnScreenDebugMessage(static_cast<uint64>(GetUniqueID()), 2.f, FColor::White, FString::Printf(TEXT("Opponent skill: %.2f"), Skill));
	}
}

void AIJPTestGameMode::CycleEra(int32 Direction)
{
	UIJPEraSubsystem* Eras = UIJPEraSubsystem::Get(this);
	if (!Eras || Eras->GetNumEras() == 0)
	{
		return;
	}

	// From an era that isn't in the list (set by code), start counting from the first.
	const int32 Current = FMath::Max(Eras->GetEraIndex(), 0);
	Eras->SetEraIndex((Current + Direction % Eras->GetNumEras() + Eras->GetNumEras()) % Eras->GetNumEras());

	const FString Name = Eras->GetEra()->DisplayName.ToString();
	UE_LOG(LogIJPong, Log, TEXT("Era: %s"), *Name);
	if (GEngine)
	{
		// Same key each time, so repeated presses replace the message instead of stacking.
		GEngine->AddOnScreenDebugMessage(static_cast<uint64>(GetUniqueID()) + 1, 2.f, FColor::White, FString::Printf(TEXT("Era: %s"), *Name));
	}
}

void AIJPTestGameMode::GetDebugLines(TArray<FString>& OutLines) const
{
	const AIJPArena* ArenaPtr = GetArena();
	OutLines.Add(TEXT("TEST MODE"));
	OutLines.Add(FString::Printf(TEXT("Opponent skill: %.2f"), ArenaPtr ? ArenaPtr->GetOpponentSkill() : 0.f));
	OutLines.Add(FString::Printf(TEXT("Your paddle: %s"), IsPlayerSideAI() ? TEXT("AI") : TEXT("you")));

	const UIJPEraSubsystem* Eras = UIJPEraSubsystem::Get(this);
	const UIJPEra* Era = Eras ? Eras->GetEra() : nullptr;
	OutLines.Add(Era
		? FString::Printf(TEXT("Era: %s (%d/%d)"), *Era->DisplayName.ToString(), Eras->GetEraIndex() + 1, Eras->GetNumEras())
		: FString(TEXT("Era: none")));

	const UIJPMatchComponent* MatchPtr = GetMatch();
	if (MatchPtr->IsOver())
	{
		OutLines.Add(FString::Printf(TEXT("Match over: %s wins"), MatchPtr->GetWinner() == EIJPSide::Left ? TEXT("left") : TEXT("right")));
	}
	else if (MatchPtr->GetRules().IsEndless())
	{
		OutLines.Add(TEXT("Match: endless"));
	}
	else
	{
		OutLines.Add(FString::Printf(TEXT("Match: first to %d"), MatchPtr->GetRules().WinTarget));
	}

	OutLines.Add(TEXT("R new match   F serve now   T AI vs AI"));
	OutLines.Add(TEXT("- / = opponent skill   [ / ] era   . (period) hide this"));
}
