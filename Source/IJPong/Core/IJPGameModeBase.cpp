// It's Just Pong

#include "Core/IJPGameModeBase.h"
#include "Abilities/IJPAbilityComponent.h"
#include "AI/IJPAIProfile.h"
#include "AI/IJPPaddleAIController.h"
#include "Core/IJPPlayerController.h"
#include "EngineUtils.h"
#include "Gameplay/IJPArena.h"
#include "Gameplay/IJPMatchComponent.h"
#include "Gameplay/IJPPaddle.h"
#include "Gameplay/IJPPaddleClass.h"
#include "Gameplay/IJPRival.h"
#include "Narrative/IJPBanterComponent.h"
#include "Narrative/IJPConversation.h"
#include "Narrative/IJPSpeechBubbleComponent.h"
#include "Narrative/IJPConversationPlayer.h"

AIJPGameModeBase::AIJPGameModeBase()
{
	PlayerControllerClass = AIJPPlayerController::StaticClass();
	AIControllerClass = AIJPPaddleAIController::StaticClass();
	// Paddles come from the arena, never from the default pawn spawn.
	DefaultPawnClass = nullptr;

	Match = CreateDefaultSubobject<UIJPMatchComponent>(TEXT("Match"));
	Conversations = CreateDefaultSubobject<UIJPConversationPlayer>(TEXT("Conversations"));
	Banter = CreateDefaultSubobject<UIJPBanterComponent>(TEXT("Banter"));
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

	Match->OnMatchEnded.AddDynamic(this, &AIJPGameModeBase::HandleMatchEnded);
	Conversations->OnFinished.AddDynamic(this, &AIJPGameModeBase::HandleConversationFinished);
	Banter->Bind(Arena, Match);
	if (AIJPPaddle* Opponent = Arena->GetPaddle(IJP::Opposite(PlayerSide)))
	{
		Opponent->GetSpeechBubble()->SetVoicePitch(DefaultOpponentVoice);
	}

	OnArenaReady();
}

void AIJPGameModeBase::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	// No Super: the default implementation spawns a DefaultPawn. The player takes a paddle instead.
	PossessPlayerPaddle(NewPlayer);
}

void AIJPGameModeBase::SetOpponentSkill(float Skill)
{
	if (!Arena)
	{
		return;
	}
	Arena->SetOpponentSkill(Skill);
	if (AIJPPaddle* Paddle = Arena->GetPaddle(IJP::Opposite(PlayerSide)))
	{
		if (AIJPPaddleAIController* AI = Cast<AIJPPaddleAIController>(Paddle->GetController()))
		{
			AI->SetSkill(Arena->GetOpponentSkill());
		}
	}
}

void AIJPGameModeBase::BeginMatch(const UIJPMatchRules* Rules)
{
	Conversations->Stop();
	const UIJPConversation* PreMatch = Rival ? UIJPConversation::PickRandom(Rival->PreMatch) : nullptr;
	Match->StartMatch(Arena, Rules, PreMatch != nullptr);
	if (PreMatch)
	{
		// HandleConversationFinished releases the serve.
		PlayConversation(PreMatch);
	}
}

void AIJPGameModeBase::PlayConversation(const UIJPConversation* Conversation)
{
	Conversations->Play(Conversation, Arena, PlayerSide);
}

void AIJPGameModeBase::HandleMatchEnded(EIJPSide Winner)
{
	if (Rival)
	{
		// Played over the winner's blinking score; nothing waits for it.
		PlayConversation(UIJPConversation::PickRandom(Winner == PlayerSide ? Rival->Loss : Rival->Win));
	}
}

void AIJPGameModeBase::HandleConversationFinished(const UIJPConversation* Conversation)
{
	Match->ReleaseServe();
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
		// Their voice in the chat bubbles.
		Paddle->GetSpeechBubble()->SetVoicePitch(Rival ? Rival->VoicePitch : DefaultOpponentVoice);
		// Their own ability in place of the class skill.
		if (Rival && Rival->RivalSkill)
		{
			Paddle->GetAbilities()->Equip(EIJPAbilitySlot::ClassSkill, Rival->RivalSkill);
		}
		Paddle->GetAbilities()->Equip(EIJPAbilitySlot::Spell, Rival ? Rival->Spell.Get() : nullptr);
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
