// It's Just Pong

#include "Core/IJPGameModeBase.h"
#include "AI/IJPAIProfile.h"
#include "AI/IJPPaddleAIController.h"
#include "Core/IJPPlayerController.h"
#include "EngineUtils.h"
#include "Gameplay/IJPArena.h"
#include "Gameplay/IJPMatchComponent.h"
#include "Gameplay/IJPPaddle.h"
#include "Gameplay/IJPPaddleClass.h"
#include "Gameplay/IJPRival.h"

AIJPGameModeBase::AIJPGameModeBase()
{
	PlayerControllerClass = AIJPPlayerController::StaticClass();
	AIControllerClass = AIJPPaddleAIController::StaticClass();
	// Paddles come from the arena, never from the default pawn spawn.
	DefaultPawnClass = nullptr;

	Match = CreateDefaultSubobject<UIJPMatchComponent>(TEXT("Match"));
}

void AIJPGameModeBase::StartPlay()
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

	// Begins play on every actor, which is when the arena spawns its paddles and ball.
	Super::StartPlay();

	if (!Arena)
	{
		return;
	}

	// Players can log in before StartPlay (PIE does), when the paddles didn't exist yet.
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		PossessPlayerPaddle(It->Get());
	}
	SpawnAIPaddle(IJP::Opposite(PlayerSide));

	OnArenaReady();
}

void AIJPGameModeBase::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	// No Super: the default implementation spawns a DefaultPawn. The player takes a paddle instead.
	PossessPlayerPaddle(NewPlayer);
}

void AIJPGameModeBase::SetRival(const UIJPRival* InRival)
{
	Rival = InRival;
	if (!Arena)
	{
		return;
	}

	const EIJPSide RivalSide = IJP::Opposite(PlayerSide);
	const UIJPPaddleClass* RivalClass = Rival ? Rival->PaddleClass.Get() : nullptr;
	Arena->SetPaddleClass(RivalSide, RivalClass ? RivalClass : Arena->GetConfiguredPaddleClass(RivalSide));

	if (AIJPPaddle* Paddle = Arena->GetPaddle(RivalSide))
	{
		if (AIJPPaddleAIController* AI = Cast<AIJPPaddleAIController>(Paddle->GetController()))
		{
			AI->SetProfile(GetAIProfileFor(RivalSide));
		}
	}
}

const UIJPAIProfile* AIJPGameModeBase::GetAIProfileFor(EIJPSide Side) const
{
	if (Rival && Rival->AIProfile && Side != PlayerSide)
	{
		return Rival->AIProfile;
	}
	return AIProfile.LoadSynchronous();
}

AIJPBall* AIJPGameModeBase::GetBall() const
{
	return Arena ? Arena->GetBall() : nullptr;
}

void AIJPGameModeBase::PossessPlayerPaddle(APlayerController* PlayerController)
{
	AIJPPaddle* Paddle = Arena ? Arena->GetPaddle(PlayerSide) : nullptr;
	if (PlayerController && Paddle && !Paddle->GetController())
	{
		PlayerController->Possess(Paddle);
	}
}

AIJPPaddleAIController* AIJPGameModeBase::SpawnAIPaddle(EIJPSide Side)
{
	AIJPPaddle* Paddle = Arena ? Arena->GetPaddle(Side) : nullptr;
	if (!Paddle || Paddle->GetController() || !AIControllerClass)
	{
		return nullptr;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AIJPPaddleAIController* AI = GetWorld()->SpawnActor<AIJPPaddleAIController>(AIControllerClass, Params);
	if (AI)
	{
		AI->SetProfile(GetAIProfileFor(Side));
		AI->SetSkill(Arena->GetOpponentSkill());
		AI->Possess(Paddle);
	}
	return AI;
}
