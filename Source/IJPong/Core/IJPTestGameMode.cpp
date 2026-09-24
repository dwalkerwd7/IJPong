// It's Just Pong

#include "Core/IJPTestGameMode.h"
#include "AI/IJPPaddleAIController.h"
#include "Core/IJPTestPlayerController.h"
#include "Gameplay/IJPArena.h"
#include "Gameplay/IJPBall.h"
#include "Gameplay/IJPPaddle.h"
#include "TimerManager.h"

AIJPTestGameMode::AIJPTestGameMode()
{
	PlayerControllerClass = AIJPTestPlayerController::StaticClass();
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

	if (AIJPBall* Ball = GetBall())
	{
		Ball->ResetBall();
		ScheduleServe(RandomSide());
	}
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

void AIJPTestGameMode::HandleGoal(EIJPSide DefendingSide)
{
	++(DefendingSide == EIJPSide::Left ? RightScore : LeftScore);
	UpdateScoreDisplay();

	// The side that just conceded receives the next serve.
	ScheduleServe(DefendingSide);
}

void AIJPTestGameMode::ScheduleServe(EIJPSide Toward)
{
	NextServeSide = Toward;
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
