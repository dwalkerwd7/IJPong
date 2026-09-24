// It's Just Pong

#include "Gameplay/IJPMatchComponent.h"
#include "Engine/World.h"
#include "Gameplay/IJPArena.h"
#include "Gameplay/IJPBall.h"
#include "Gameplay/IJPMatchRules.h"
#include "TimerManager.h"

namespace
{
	EIJPSide RandomSide() { return FMath::RandBool() ? EIJPSide::Left : EIJPSide::Right; }
}

void UIJPMatchComponent::StartMatch(AIJPArena* InArena, const UIJPMatchRules* InRules)
{
	Arena = InArena;
	Rules = InRules;

	AIJPBall* Ball = GetBall();
	if (!Ball)
	{
		UE_LOG(LogIJPong, Warning, TEXT("Can't start a match without an arena and ball."));
		return;
	}
	if (BoundBall != Ball)
	{
		if (BoundBall.IsValid())
		{
			BoundBall->OnGoal.RemoveDynamic(this, &UIJPMatchComponent::HandleGoal);
		}
		Ball->OnGoal.AddDynamic(this, &UIJPMatchComponent::HandleGoal);
		BoundBall = Ball;
	}

	LeftScore = RightScore = 0;
	UpdateScoreDisplay();
	Arena->ClearWinner();

	// Abandons any rally: the ball goes back to blinking at the centre.
	ScheduleServe(RandomSide());
}

void UIJPMatchComponent::ServeNow()
{
	if (!IsOver() && GetBall())
	{
		GetWorld()->GetTimerManager().ClearTimer(ServeTimer);
		Serve(RandomSide());
	}
}

const UIJPMatchRules& UIJPMatchComponent::GetRules() const
{
	return Rules ? *Rules : *GetDefault<UIJPMatchRules>();
}

void UIJPMatchComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ServeTimer);
	}
	Super::EndPlay(EndPlayReason);
}

void UIJPMatchComponent::HandleGoal(EIJPSide DefendingSide)
{
	if (IsOver())
	{
		return;
	}

	const EIJPSide Scorer = IJP::Opposite(DefendingSide);
	int32& ScorerScore = Scorer == EIJPSide::Left ? LeftScore : RightScore;
	++ScorerScore;
	UpdateScoreDisplay();

	if (!GetRules().IsEndless() && ScorerScore >= GetRules().WinTarget)
	{
		EndMatch(Scorer);
		return;
	}

	Arena->FlashScore(Scorer);
	// The side that just conceded receives the next serve.
	ScheduleServe(DefendingSide);
}

void UIJPMatchComponent::ScheduleServe(EIJPSide Toward)
{
	Phase = EIJPMatchPhase::Serve;
	NextServeSide = Toward;
	GetBall()->BlinkAtCentre();
	GetWorld()->GetTimerManager().SetTimer(ServeTimer, this, &UIJPMatchComponent::ServeBall, FMath::Max(GetRules().ServeDelay, UE_KINDA_SMALL_NUMBER));
}

void UIJPMatchComponent::ServeBall()
{
	AIJPBall* Ball = GetBall();
	if (Ball && !Ball->IsInPlay())
	{
		Serve(NextServeSide);
	}
}

void UIJPMatchComponent::Serve(EIJPSide Toward)
{
	const float MaxAngle = GetRules().MaxServeAngleDeg;
	GetBall()->Serve(Toward, FMath::FRandRange(-MaxAngle, MaxAngle));
	Phase = EIJPMatchPhase::Rally;
}

void UIJPMatchComponent::EndMatch(EIJPSide InWinner)
{
	Phase = EIJPMatchPhase::MatchOver;
	Winner = InWinner;
	GetWorld()->GetTimerManager().ClearTimer(ServeTimer);
	GetBall()->ResetBall();
	Arena->ShowWinner(Winner);

	UE_LOG(LogIJPong, Log, TEXT("Match over: %s wins %d-%d."), Winner == EIJPSide::Left ? TEXT("left") : TEXT("right"), LeftScore, RightScore);
	OnMatchEnded.Broadcast(Winner);
}

void UIJPMatchComponent::UpdateScoreDisplay() const
{
	Arena->SetScore(EIJPSide::Left, LeftScore);
	Arena->SetScore(EIJPSide::Right, RightScore);
}

AIJPBall* UIJPMatchComponent::GetBall() const
{
	return Arena ? Arena->GetBall() : nullptr;
}
