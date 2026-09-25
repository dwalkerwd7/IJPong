// It's Just Pong

#include "Narrative/IJPConversationPlayer.h"
#include "Engine/World.h"
#include "Gameplay/IJPArena.h"
#include "Gameplay/IJPPaddle.h"
#include "Narrative/IJPConversation.h"
#include "Narrative/IJPSpeechBubbleComponent.h"
#include "TimerManager.h"

void UIJPConversationPlayer::Play(const UIJPConversation* Conversation, AIJPArena* InArena, EIJPSide InPlayerSide)
{
	Stop();
	if (!Conversation || !InArena)
	{
		return;
	}

	Current = Conversation;
	Arena = InArena;
	PlayerSide = InPlayerSide;
	PlayLine(0);
}

void UIJPConversationPlayer::Stop()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(GapTimer);
	}
	if (Speaking.IsValid())
	{
		Speaking->Hide();
	}
	Unbind();
	Current = nullptr;
}

void UIJPConversationPlayer::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Stop();
	Super::EndPlay(EndPlayReason);
}

void UIJPConversationPlayer::PlayLine(int32 Index)
{
	LineIndex = Index;
	if (!Current->Lines.IsValidIndex(Index))
	{
		// Out of lines (or none to begin with): done.
		const UIJPConversation* Finished = Current;
		Current = nullptr;
		OnFinished.Broadcast(Finished);
		return;
	}

	const FIJPConversationLine& Line = Current->Lines[Index];
	const EIJPSide Side = Line.Speaker == EIJPSpeaker::Player ? PlayerSide : IJP::Opposite(PlayerSide);
	AIJPPaddle* Paddle = Arena.IsValid() ? Arena->GetPaddle(Side) : nullptr;
	if (!Paddle)
	{
		PlayLine(Index + 1);
		return;
	}

	Speaking = Paddle->GetSpeechBubble();
	Speaking->OnLineFinished.AddDynamic(this, &UIJPConversationPlayer::HandleLineFinished);
	Speaking->Say(Line.Text, Line.HoldTime);
}

void UIJPConversationPlayer::HandleLineFinished()
{
	Unbind();
	if (GapBetweenLines > 0.f)
	{
		GetWorld()->GetTimerManager().SetTimer(GapTimer, this, &UIJPConversationPlayer::PlayNextLine, GapBetweenLines);
	}
	else
	{
		PlayNextLine();
	}
}

void UIJPConversationPlayer::PlayNextLine()
{
	if (Current)
	{
		PlayLine(LineIndex + 1);
	}
}

void UIJPConversationPlayer::Unbind()
{
	if (Speaking.IsValid())
	{
		Speaking->OnLineFinished.RemoveDynamic(this, &UIJPConversationPlayer::HandleLineFinished);
	}
	Speaking = nullptr;
}
