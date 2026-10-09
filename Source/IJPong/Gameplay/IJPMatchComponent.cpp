// It's Just Pong

#include "Gameplay/IJPMatchComponent.h"
#include "Engine/World.h"
#include "Gameplay/IJPArena.h"
#include "Gameplay/IJPBall.h"
#include "Gameplay/IJPBallType.h"
#include "Gameplay/IJPMatchRules.h"
#include "Presentation/IJPServeCueComponent.h"
#include "TimerManager.h"

namespace
{
	EIJPSide RandomSide() { return FMath::RandBool() ? EIJPSide::Left : EIJPSide::Right; }
}

void UIJPMatchComponent::StartMatch(AIJPArena* InArena, const UIJPMatchRules* InRules, bool bHoldServe)
{
	Arena = InArena;
	Rules = InRules;
	bServeHeld = bHoldServe;
	++MatchNumber;

	if (!GetBall())
	{
		UE_LOG(LogIJPong, Warning, TEXT("Can't start a match without an arena and ball."));
		return;
	}
	if (BoundArena != Arena)
	{
		if (BoundArena.IsValid())
		{
			BoundArena->OnBallGoal.RemoveDynamic(this, &UIJPMatchComponent::HandleGoal);
		}
		Arena->OnBallGoal.AddDynamic(this, &UIJPMatchComponent::HandleGoal);
		BoundArena = Arena;
	}

	for (int32 i = 0; i < 2; ++i)
	{
		Health[i] = MaxHealth[i] = GetRules().StartingHealth;
		Goals[i] = 0;
	}
	UpdateScoreDisplay(true);
	Arena->ClearWinner();

	// Abandons any rally: every ball leaves play and the main one blinks at the centre.
	ScheduleServe(RandomSide());
}

AIJPBall* UIJPMatchComponent::LaunchExtraBall(const UIJPBallType* Type)
{
	if (Phase != EIJPMatchPhase::Serve && Phase != EIJPMatchPhase::Rally)
	{
		return nullptr;
	}

	AIJPBall* Extra = Arena->AddBall(Type);
	if (Extra)
	{
		Extra->Serve(RandomSide(), RandomServeAngle());
	}
	return Extra;
}

void UIJPMatchComponent::StopMatch()
{
	GetWorld()->GetTimerManager().ClearTimer(ServeTimer);
	CancelRefill();
	bServeHeld = false;
	Phase = EIJPMatchPhase::None;
	if (Arena)
	{
		Arena->ResetBalls();
	}
}

void UIJPMatchComponent::ReleaseServe()
{
	if (!bServeHeld)
	{
		return;
	}
	bServeHeld = false;
	if (Phase == EIJPMatchPhase::Serve && !GetWorld()->GetTimerManager().IsTimerActive(ServeTimer))
	{
		ScheduleServe(NextServeSide);
	}
}

void UIJPMatchComponent::ServeNow()
{
	if (!IsOver() && GetBall())
	{
		bServeHeld = false;
		GetWorld()->GetTimerManager().ClearTimer(ServeTimer);
		CancelRefill();
		Arena->ResetBalls();
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
		World->GetTimerManager().ClearTimer(RefillTimer);
	}
	Super::EndPlay(EndPlayReason);
}

void UIJPMatchComponent::ApplyDamage(EIJPSide Side, float Amount)
{
	if (!IsPlaying() || GetRules().IsEndless() || Amount <= 0.f)
	{
		return;
	}

	float& SideHealth = Health[SideIndex(Side)];
	SideHealth = FMath::Max(SideHealth - Amount, 0.f);
	UpdateScoreDisplay();
	OnHealthChanged.Broadcast(Side, SideHealth, Amount);

	// A listener may have stopped the match (e.g. the run ending with the player's health).
	if (!IsPlaying())
	{
		return;
	}
	if (SideHealth <= 0.f)
	{
		EndMatch(IJP::Opposite(Side));
		return;
	}
	Arena->FlashScore(Side);
}

void UIJPMatchComponent::Heal(EIJPSide Side, float Amount)
{
	if (!IsPlaying() || GetRules().IsEndless() || Amount <= 0.f)
	{
		return;
	}
	const int32 Index = SideIndex(Side);
	const float Healed = FMath::Min(Health[Index] + Amount, MaxHealth[Index]) - Health[Index];
	if (Healed <= 0.f)
	{
		return;
	}
	Health[Index] += Healed;
	UpdateScoreDisplay();
	OnHealthChanged.Broadcast(Side, Health[Index], -Healed);
}

void UIJPMatchComponent::SetHealth(EIJPSide Side, float InHealth, float InMaxHealth)
{
	const int32 Index = SideIndex(Side);
	MaxHealth[Index] = FMath::Max(InMaxHealth, 0.f);
	Health[Index] = FMath::Clamp(InHealth, 0.f, MaxHealth[Index]);
	UpdateScoreDisplay(true);
}

void UIJPMatchComponent::HandleGoal(AIJPBall* ScoringBall, EIJPSide DefendingSide)
{
	if (!IsPlaying())
	{
		return;
	}

	const EIJPSide Scorer = IJP::Opposite(DefendingSide);
	const int32 Points = ScoringBall ? ScoringBall->GetType().Points : 1;
	++Goals[SideIndex(Scorer)];

	if (GetRules().IsEndless())
	{
		// No health: the numbers count goals, 1972 style.
		UpdateScoreDisplay();
		Arena->FlashScore(Scorer);
	}
	else
	{
		ApplyDamage(DefendingSide, Points * GetRules().GoalDamage * (ScoringBall ? ScoringBall->GetDamageScale() : 1.f));
		if (!IsPlaying())
		{
			return;
		}
		// Leech: the scorer drinks some back.
		if (ScoringBall && ScoringBall->GetType().HealOnGoal > 0.f)
		{
			Heal(Scorer, ScoringBall->GetType().HealOnGoal);
		}
	}
	OnPointScored.Broadcast(Scorer, Points);

	// Serve again only once the court is empty, and only if a serve isn't already on its way
	// (a ball launched during the wait before a serve can score first).
	if (Arena->GetNumBallsInPlay() == 0 && !GetWorld()->GetTimerManager().IsTimerActive(ServeTimer))
	{
		// The side that just conceded receives the next serve.
		ScheduleServe(DefendingSide);
	}
	else
	{
		MaybeRefill(DefendingSide);
	}
}

void UIJPMatchComponent::MaybeRefill(EIJPSide Toward)
{
	// Only mid-rally, one refill at a time, and only while there's room under the limit.
	const int32 Limit = GetRules().RefillBallLimit;
	if (Phase != EIJPMatchPhase::Rally || Limit <= 0 || RefillBall.IsValid() || Arena->GetNumBallsInPlay() >= Limit)
	{
		return;
	}

	// Only when every ball left is one that calls for company (a Heavy); anything else keeps the
	// rally going as it is.
	const UIJPBallType* MainType = GetServedType(0);
	for (const AIJPBall* Each : Arena->GetBalls())
	{
		if (Each->IsInPlay() && (&Each->GetType() == MainType || !Each->GetType().bRefillWhenAlone))
		{
			return;
		}
	}

	// Waiting at the centre keeps AddBall from handing it to anything else; the cue shows it coming.
	AIJPBall* Refill = Arena->AddBall(MainType);
	if (!Refill)
	{
		return;
	}
	Arena->GetServeCue()->Start(Refill, GetRules().ServeDelay);
	RefillBall = Refill;
	RefillSide = Toward;
	GetWorld()->GetTimerManager().SetTimer(RefillTimer, this, &UIJPMatchComponent::ServeRefill, FMath::Max(GetRules().ServeDelay, UE_KINDA_SMALL_NUMBER));
}

void UIJPMatchComponent::ServeRefill()
{
	AIJPBall* Refill = RefillBall.Get();
	RefillBall.Reset();
	// Still waiting (nothing reset it, e.g. a full serve) and the match goes on.
	if (Refill && Refill->IsBlinking() && Phase == EIJPMatchPhase::Rally)
	{
		Refill->Serve(RefillSide, RandomServeAngle());
	}
}

void UIJPMatchComponent::CancelRefill()
{
	GetWorld()->GetTimerManager().ClearTimer(RefillTimer);
	RefillBall.Reset();
}

void UIJPMatchComponent::ScheduleServe(EIJPSide Toward)
{
	Phase = EIJPMatchPhase::Serve;
	NextServeSide = Toward;
	CancelRefill();
	Arena->ResetBalls();
	// The main ball waits at the centre as what's about to be served first.
	GetBall()->SetType(GetServedType(0));
	if (bServeHeld)
	{
		// Blinking until ReleaseServe() starts the countdown. Cancel any from before (e.g. a match restarted mid-countdown).
		GetBall()->BlinkAtCentre();
		GetWorld()->GetTimerManager().ClearTimer(ServeTimer);
		return;
	}
	// The ring fills around it, then it blinks and beeps, then the serve.
	Arena->GetServeCue()->Start(GetBall(), GetRules().ServeDelay);
	GetWorld()->GetTimerManager().SetTimer(ServeTimer, this, &UIJPMatchComponent::ServeBall, FMath::Max(GetRules().ServeDelay, UE_KINDA_SMALL_NUMBER));
}

void UIJPMatchComponent::ServeBall()
{
	AIJPBall* Ball = GetBall();
	if (Ball && !Ball->IsInPlay())
	{
		Serve(NextServeSide);
	}
	else
	{
		// Something already served it (ServeNow, an ability, a test): the rally is under way.
		Phase = EIJPMatchPhase::Rally;
	}
}

void UIJPMatchComponent::Serve(EIJPSide Toward)
{
	// The main ball goes first: once it's in play, AddBall won't hand it out again.
	const int32 NumBalls = FMath::Max(GetRules().ServedBalls.Num(), 1) + ExtraServedBalls.Num();
	for (int32 i = 0; i < NumBalls; ++i)
	{
		AIJPBall* Ball = i == 0 ? GetBall() : Arena->AddBall(GetServedType(i));
		if (!Ball)
		{
			continue;
		}
		Ball->SetType(GetServedType(i));
		Ball->Serve(i % 2 == 0 ? Toward : IJP::Opposite(Toward), RandomServeAngle());
	}
	Phase = EIJPMatchPhase::Rally;
}

const UIJPBallType* UIJPMatchComponent::GetServedType(int32 Index) const
{
	// The rules' serve list (or one default ball) first, then any extras.
	const TArray<TObjectPtr<UIJPBallType>>& Served = GetRules().ServedBalls;
	const int32 RulesCount = FMath::Max(Served.Num(), 1);
	const UIJPBallType* Type = Index < RulesCount
		? (Served.IsValidIndex(Index) ? Served[Index].Get() : nullptr)
		: (ExtraServedBalls.IsValidIndex(Index - RulesCount) ? ExtraServedBalls[Index - RulesCount].Get() : nullptr);
	return Type ? Type : Arena->GetDefaultBallType();
}

float UIJPMatchComponent::RandomServeAngle() const
{
	const float MaxAngle = GetRules().MaxServeAngleDeg;
	return FMath::FRandRange(-MaxAngle, MaxAngle);
}

void UIJPMatchComponent::EndMatch(EIJPSide InWinner)
{
	Phase = EIJPMatchPhase::MatchOver;
	Winner = InWinner;
	GetWorld()->GetTimerManager().ClearTimer(ServeTimer);
	CancelRefill();
	Arena->ResetBalls();
	Arena->ShowWinner(Winner);

	UE_LOG(LogIJPong, Log, TEXT("Match over: %s wins, health %.1f-%.1f."), Winner == EIJPSide::Left ? TEXT("left") : TEXT("right"), Health[0], Health[1]);
	OnMatchEnded.Broadcast(Winner);
}

void UIJPMatchComponent::UpdateScoreDisplay(bool bInstant) const
{
	// Health rounded up (a side with any left never reads 0), or goals when there's no health.
	const bool bEndless = GetRules().IsEndless();
	for (EIJPSide Side : { EIJPSide::Left, EIJPSide::Right })
	{
		const int32 Index = SideIndex(Side);
		Arena->SetScore(Side, bEndless ? Goals[Index] : FMath::CeilToInt(Health[Index]));
		Arena->SetHealthDisplay(Side, bEndless ? 0.f : Health[Index], bEndless ? 0.f : MaxHealth[Index], bInstant);
	}
}

AIJPBall* UIJPMatchComponent::GetBall() const
{
	return Arena ? Arena->GetBall() : nullptr;
}
