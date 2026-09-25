// It's Just Pong

#include "Narrative/IJPBanterComponent.h"
#include "Core/IJPGameModeBase.h"
#include "Engine/World.h"
#include "Gameplay/IJPArena.h"
#include "Gameplay/IJPBall.h"
#include "Gameplay/IJPMatchComponent.h"
#include "Gameplay/IJPMatchRules.h"
#include "Narrative/IJPConversation.h"

void UIJPBanterComponent::Bind(AIJPArena* InArena, UIJPMatchComponent* InMatch)
{
	Arena = InArena;
	Match = InMatch;
	if (InArena)
	{
		InArena->OnBallReturned.AddDynamic(this, &UIJPBanterComponent::HandleBallReturned);
	}
	if (InMatch)
	{
		InMatch->OnPointScored.AddDynamic(this, &UIJPBanterComponent::HandlePointScored);
	}
}

AIJPGameModeBase* UIJPBanterComponent::GetGameMode() const
{
	return Cast<AIJPGameModeBase>(GetOwner());
}

void UIJPBanterComponent::HandlePointScored(EIJPSide Scorer, int32 Points)
{
	const UIJPMatchComponent* MatchPtr = Match.Get();
	if (!MatchPtr)
	{
		return;
	}

	const bool bPlayerScored = Scorer == AIJPGameModeBase::PlayerSide;
	TArray<EIJPBanterEvent> Events;

	// Match point: one more goal from the scorer ends it.
	const UIJPMatchRules& Rules = MatchPtr->GetRules();
	if (!Rules.IsEndless() && MatchPtr->GetHealth(IJP::Opposite(Scorer)) <= Rules.GoalDamage)
	{
		Events.Add(bPlayerScored ? EIJPBanterEvent::PlayerMatchPoint : EIJPBanterEvent::RivalMatchPoint);
	}
	// This was the match's first goal.
	if (MatchPtr->GetGoals(EIJPSide::Left) + MatchPtr->GetGoals(EIJPSide::Right) == 1)
	{
		Events.Add(bPlayerScored ? EIJPBanterEvent::PlayerScoredFirst : EIJPBanterEvent::RivalScoredFirst);
	}
	Events.Add(bPlayerScored ? EIJPBanterEvent::PlayerScored : EIJPBanterEvent::RivalScored);
	TryBanter(Events);
}

void UIJPBanterComponent::HandleBallReturned(AIJPBall* Ball, AIJPPaddle* Paddle)
{
	const AIJPGameModeBase* GameMode = GetGameMode();
	const UIJPRival* Rival = GameMode ? GameMode->GetRival() : nullptr;
	if (Rival && Ball && Ball->GetRallyHits() == Rival->LongRallyReturns)
	{
		TryBanter({ EIJPBanterEvent::LongRally });
	}
}

void UIJPBanterComponent::TryBanter(const TArray<EIJPBanterEvent>& ByPriority)
{
	AIJPGameModeBase* GameMode = GetGameMode();
	const UIJPRival* Rival = GameMode ? GameMode->GetRival() : nullptr;
	const UIJPMatchComponent* MatchPtr = Match.Get();
	if (!Rival || !MatchPtr)
	{
		return;
	}

	// Not over the pre-match talk (serve still held), not once the match is decided, not too often.
	const double Now = GetWorld()->GetTimeSeconds();
	if (MatchPtr->IsServeHeld() || MatchPtr->IsOver() || Now - LastBanterTime < Rival->BanterCooldown)
	{
		return;
	}

	for (const EIJPBanterEvent Event : ByPriority)
	{
		if (const FIJPBanterLines* Lines = Rival->FindBanter(Event))
		{
			// Only the most important moment with lines gets its chance; lesser ones don't get a second try.
			if (FMath::FRand() < Lines->Chance)
			{
				GameMode->PlayConversation(UIJPConversation::PickRandom(Lines->Conversations));
				LastBanterTime = Now;
				++BanterCount;
			}
			return;
		}
	}
}
