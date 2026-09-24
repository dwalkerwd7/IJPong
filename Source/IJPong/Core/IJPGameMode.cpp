// It's Just Pong

#include "Core/IJPGameMode.h"
#include "Core/IJPPlayerController.h"
#include "Core/IJPTypes.h"
#include "EngineUtils.h"
#include "Gameplay/IJPArena.h"
#include "Gameplay/IJPBall.h"
#include "Gameplay/IJPPaddle.h"
#include "TimerManager.h"

AIJPGameMode::AIJPGameMode()
{
	PlayerControllerClass = AIJPPlayerController::StaticClass();
	// Paddles come from the arena, never from the default pawn spawn.
	DefaultPawnClass = nullptr;
}

void AIJPGameMode::StartPlay()
{
	for (TActorIterator<AIJPArena> It(GetWorld()); It; ++It)
	{
		if (Arena)
		{
			UE_LOG(LogIJPong, Warning, TEXT("More than one IJPArena in the level; using %s."), *Arena->GetName());
			break;
		}
		Arena = *It;
	}
	if (!Arena)
	{
		UE_LOG(LogIJPong, Warning, TEXT("No IJPArena in the level."));
	}

	// Begins play on every actor, which is when the arena spawns its paddles.
	Super::StartPlay();

	// Players can log in before StartPlay (PIE does), when the paddles didn't exist yet.
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		PossessPlayerPaddle(It->Get());
	}

	if (AIJPBall* Ball = Arena ? Arena->GetBall() : nullptr)
	{
		Arena->SetScore(EIJPSide::Left, LeftScore);
		Arena->SetScore(EIJPSide::Right, RightScore);
		Ball->OnGoal.AddDynamic(this, &AIJPGameMode::HandleGoal);
		ScheduleServe(FMath::RandBool() ? EIJPSide::Left : EIJPSide::Right);
	}
}

void AIJPGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	// No Super: the default implementation spawns a DefaultPawn. The player takes a paddle instead.
	PossessPlayerPaddle(NewPlayer);
}

void AIJPGameMode::PossessPlayerPaddle(APlayerController* PlayerController)
{
	if (!Arena || !PlayerController)
	{
		return;
	}

	AIJPPaddle* Paddle = Arena->GetPaddle(EIJPSide::Left);
	if (Paddle && !Paddle->GetController())
	{
		PlayerController->Possess(Paddle);
	}
}

void AIJPGameMode::HandleGoal(EIJPSide DefendingSide)
{
	int32& Score = DefendingSide == EIJPSide::Left ? RightScore : LeftScore;
	++Score;
	Arena->SetScore(IJP::Opposite(DefendingSide), Score);

	// The side that just conceded receives the next serve.
	ScheduleServe(DefendingSide);
}

void AIJPGameMode::ScheduleServe(EIJPSide Toward)
{
	NextServeSide = Toward;
	GetWorldTimerManager().SetTimer(ServeTimer, this, &AIJPGameMode::ServeBall, FMath::Max(ServeDelay, UE_KINDA_SMALL_NUMBER));
}

void AIJPGameMode::ServeBall()
{
	AIJPBall* Ball = Arena ? Arena->GetBall() : nullptr;
	if (Ball && !Ball->IsInPlay())
	{
		Ball->Serve(NextServeSide, FMath::FRandRange(-MaxServeAngleDeg, MaxServeAngleDeg));
	}
}
