// It's Just Pong

#include "Core/IJPTestGameMode.h"
#include "AI/IJPPaddleAIController.h"
#include "Core/IJPTestHUD.h"
#include "Core/IJPTestPlayerController.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "Gameplay/IJPArena.h"
#include "Gameplay/IJPBall.h"
#include "Gameplay/IJPPaddle.h"
#include "TimerManager.h"

AIJPTestGameMode::AIJPTestGameMode()
{
	PlayerControllerClass = AIJPTestPlayerController::StaticClass();
	HUDClass = AIJPTestHUD::StaticClass();
}

void AIJPTestGameMode::OnArenaReady()
{
	if (AIJPBall* Ball = GetBall())
	{
		Ball->OnGoal.AddDynamic(this, &AIJPTestGameMode::HandleGoal);
		UpdateScoreDisplay();
		ScheduleServe(RandomSide());
	}
}

void AIJPTestGameMode::ResetScore()
{
	LeftScore = RightScore = 0;
	UpdateScoreDisplay();

	// Abandons any rally: the ball goes back to blinking at the centre.
	ScheduleServe(RandomSide());
}

void AIJPTestGameMode::ServeNow()
{
	if (AIJPBall* Ball = GetBall())
	{
		GetWorldTimerManager().ClearTimer(ServeTimer);
		Ball->Serve(RandomSide(), FMath::FRandRange(-MaxServeAngleDeg, MaxServeAngleDeg));
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

void AIJPTestGameMode::GetDebugLines(TArray<FString>& OutLines) const
{
	const AIJPArena* ArenaPtr = GetArena();
	OutLines.Add(TEXT("TEST MODE"));
	OutLines.Add(FString::Printf(TEXT("Opponent skill: %.2f"), ArenaPtr ? ArenaPtr->GetOpponentSkill() : 0.f));
	OutLines.Add(FString::Printf(TEXT("Your paddle: %s"), IsPlayerSideAI() ? TEXT("AI") : TEXT("you")));
	OutLines.Add(TEXT("R reset score   F serve now   T AI vs AI"));
	OutLines.Add(TEXT("- / = opponent skill   . (period) hide this"));
}

void AIJPTestGameMode::HandleGoal(EIJPSide DefendingSide)
{
	++(DefendingSide == EIJPSide::Left ? RightScore : LeftScore);
	UpdateScoreDisplay();
	GetArena()->FlashScore(IJP::Opposite(DefendingSide));

	// The side that just conceded receives the next serve.
	ScheduleServe(DefendingSide);
}

void AIJPTestGameMode::ScheduleServe(EIJPSide Toward)
{
	NextServeSide = Toward;
	if (AIJPBall* Ball = GetBall())
	{
		Ball->BlinkAtCentre();
	}
	GetWorldTimerManager().SetTimer(ServeTimer, this, &AIJPTestGameMode::ServeBall, FMath::Max(ServeDelay, UE_KINDA_SMALL_NUMBER));
}

void AIJPTestGameMode::ServeBall()
{
	AIJPBall* Ball = GetBall();
	if (Ball && !Ball->IsInPlay())
	{
		Ball->Serve(NextServeSide, FMath::FRandRange(-MaxServeAngleDeg, MaxServeAngleDeg));
	}
}

void AIJPTestGameMode::UpdateScoreDisplay() const
{
	if (AIJPArena* ArenaPtr = GetArena())
	{
		ArenaPtr->SetScore(EIJPSide::Left, LeftScore);
		ArenaPtr->SetScore(EIJPSide::Right, RightScore);
	}
}
