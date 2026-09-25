// It's Just Pong

#include "Core/IJPRunGameMode.h"
#include "Abilities/IJPAbility.h"
#include "Abilities/IJPAbilityComponent.h"
#include "Engine/World.h"
#include "Gameplay/IJPArena.h"
#include "Gameplay/IJPMatchComponent.h"
#include "Gameplay/IJPPaddle.h"
#include "Gameplay/IJPRival.h"
#include "GameFramework/PlayerController.h"
#include "Narrative/IJPConversationPlayer.h"
#include "Run/IJPActConfig.h"
#include "Run/IJPReward.h"
#include "Run/IJPRunMapView.h"
#include "Run/IJPRunSubsystem.h"
#include "TimerManager.h"

void AIJPRunGameMode::OnArenaReady()
{
	AIJPArena* ArenaPtr = GetArena();

	// The map gets its own spot in the level, well away from the arena, with its own camera.
	FActorSpawnParameters Params;
	Params.Owner = this;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	const FVector MapLocation = ArenaPtr->GetActorLocation() + ArenaPtr->GetActorUpVector() * 5000.f;
	MapView = GetWorld()->SpawnActor<AIJPRunMapView>(AIJPRunMapView::StaticClass(), MapLocation, ArenaPtr->GetActorRotation(), Params);
	MapView->Init(ArenaPtr);

	ArenaPtr->OnBallGoal.AddDynamic(this, &AIJPRunGameMode::HandleBallGoal);
	GetMatch()->OnMatchEnded.AddDynamic(this, &AIJPRunGameMode::HandleRunMatchEnded);

	StartNewRun();
}

void AIJPRunGameMode::StartNewRun(const UIJPActConfig* Act, int32 Seed, int32 InStartingHealth)
{
	UIJPRunSubsystem* Run = UIJPRunSubsystem::Get(this);
	if (!Run)
	{
		return;
	}

	GetWorld()->GetTimerManager().ClearTimer(AfterMatchTimer);
	GetMatch()->StopMatch();
	GetConversations()->Stop();
	SetRival(nullptr);

	RunAct = Act ? Act : FirstAct.LoadSynchronous();
	RunStartingHealth = InStartingHealth > 0 ? InStartingHealth : StartingHealth;
	Run->StartRun(RunAct, Seed == INDEX_NONE ? FMath::Rand() : Seed, RunStartingHealth);
	ApplyLoadout(); // a fresh run: nothing gathered yet
	Phase = EIJPRunPhase::Map;
	ShowMap();
}

bool AIJPRunGameMode::HandleUIStep(int32 Direction)
{
	if (Phase != EIJPRunPhase::Map && Phase != EIJPRunPhase::Reward)
	{
		return false;
	}
	MapView->Step(Direction);
	return true;
}

bool AIJPRunGameMode::HandleUIConfirm()
{
	switch (Phase)
	{
	case EIJPRunPhase::Map:
		EnterSelectedNode();
		return true;
	case EIJPRunPhase::Reward:
	{
		// The cards are the offer, left to right, then "skip".
		UIJPRunSubsystem* Run = UIJPRunSubsystem::Get(this);
		const int32 Card = MapView->GetSelectedCard();
		Run->TakeReward(Run->GetOffer().IsValidIndex(Card) ? Card : INDEX_NONE);
		Phase = EIJPRunPhase::Map;
		ShowMap();
		return true;
	}
	case EIJPRunPhase::Ended:
		// Same act and health as the run that just ended, on a fresh map.
		StartNewRun(RunAct, INDEX_NONE, RunStartingHealth);
		return true;
	default:
		// Mid-match the button is the class skill again.
		return false;
	}
}

void AIJPRunGameMode::EnterSelectedNode()
{
	UIJPRunSubsystem* Run = UIJPRunSubsystem::Get(this);
	const int32 Node = MapView->GetSelectedNode();
	if (!Run->EnterNode(Node))
	{
		return;
	}

	const EIJPNodeType Type = Run->GetMap().Nodes[Node].Type;
	const FIJPEncounter* Encounter = Run->GetAct()->GetEncounter(Type);
	if (!Encounter)
	{
		// Rest (done on entry): stay on the map.
		ShowMap();
		return;
	}

	// A fight: the act's rival and difficulty for this kind of node.
	const UIJPRival* NodeRival = Encounter->Rivals.IsEmpty() ? nullptr : Encounter->Rivals[FMath::RandHelper(Encounter->Rivals.Num())].Get();
	SetRival(NodeRival);
	SetOpponentSkill(Encounter->Skill);
	ApplyLoadout();
	BeginMatch(Encounter->Rules);
	Phase = EIJPRunPhase::Playing;
	ShowArena();
}

void AIJPRunGameMode::HandleBallGoal(AIJPBall* ScoringBall, EIJPSide DefendingSide)
{
	// AfterMatch too: a match-winning goal may reach the match (which ends it) before it reaches us.
	const bool bFighting = Phase == EIJPRunPhase::Playing || Phase == EIJPRunPhase::AfterMatch;
	if (!bFighting || DefendingSide != PlayerSide)
	{
		return;
	}

	// Every goal against the player costs a point of run health, even in a match they go on to win.
	UIJPRunSubsystem* Run = UIJPRunSubsystem::Get(this);
	Run->LoseHealth(1);
	if (Run->GetState() == EIJPRunState::Lost)
	{
		EndRun();
	}
}

void AIJPRunGameMode::HandleRunMatchEnded(EIJPSide Winner)
{
	if (Phase != EIJPRunPhase::Playing)
	{
		return;
	}
	bLastMatchWon = Winner == PlayerSide;
	Phase = EIJPRunPhase::AfterMatch;
	GetWorld()->GetTimerManager().SetTimer(AfterMatchTimer, this, &AIJPRunGameMode::FinishNode, FMath::Max(PostMatchDelay, UE_KINDA_SMALL_NUMBER));
}

void AIJPRunGameMode::FinishNode()
{
	UIJPRunSubsystem* Run = UIJPRunSubsystem::Get(this);
	Run->CompleteNode(bLastMatchWon);
	GetMatch()->StopMatch();
	GetConversations()->Stop();

	if (Run->GetState() == EIJPRunState::Running && Run->HasOffer())
	{
		Phase = EIJPRunPhase::Reward;
		ShowRewards();
	}
	else if (Run->GetState() == EIJPRunState::Running)
	{
		Phase = EIJPRunPhase::Map;
		ShowMap();
	}
	else
	{
		EndRun();
	}
}

void AIJPRunGameMode::EndRun()
{
	GetWorld()->GetTimerManager().ClearTimer(AfterMatchTimer);
	GetMatch()->StopMatch();
	GetConversations()->Stop();
	Phase = EIJPRunPhase::Ended;
	ShowMap();
}

void AIJPRunGameMode::ShowMap()
{
	const UIJPRunSubsystem* Run = UIJPRunSubsystem::Get(this);
	MapView->Refresh();
	if (Phase == EIJPRunPhase::Ended)
	{
		MapView->SetFooter(Run->GetState() == EIJPRunState::Won
			? TEXT("ACT CLEARED!    SPACE: NEW RUN")
			: TEXT("RUN OVER    SPACE: NEW RUN"));
	}
	else
	{
		MapView->SetFooter(TEXT("A / D  CHOOSE    SPACE  GO"));
	}
	SetViewTarget(MapView);
}

void AIJPRunGameMode::ApplyLoadout()
{
	const UIJPRunSubsystem* Run = UIJPRunSubsystem::Get(this);
	AIJPPaddle* Paddle = GetArena() ? GetArena()->GetPaddle(PlayerSide) : nullptr;
	if (!Run || !Paddle)
	{
		return;
	}

	const FIJPRunLoadout& Loadout = Run->GetLoadout();
	Paddle->SetRunScales(1.f + Loadout.PaddleLengthBonus, 1.f + Loadout.PaddleSpeedBonus);
	UIJPAbilityComponent* Abilities = Paddle->GetAbilities();
	Abilities->SetCooldownScale(EIJPAbilitySlot::ClassSkill, FMath::Max(1.f - Loadout.ClassSkillCooldownCut, 0.1f));
	Abilities->Equip(EIJPAbilitySlot::RunAbility, Loadout.RunAbility);
	GetMatch()->SetExtraServedBalls(Loadout.ExtraServedBalls);
}

void AIJPRunGameMode::ShowRewards()
{
	const UIJPRunSubsystem* Run = UIJPRunSubsystem::Get(this);
	TArray<AIJPRunMapView::FCard> Cards;
	for (const UIJPReward* Reward : Run->GetOffer())
	{
		Cards.Add({ Reward->DisplayName.ToString().ToUpper(), Reward->Description.ToString() });
	}
	Cards.Add({ TEXT("SKIP"), FString::Printf(TEXT("+%d COINS"), Run->GetAct()->SkipCoins) });

	MapView->ShowCards(FString::Printf(TEXT("PICK A REWARD    HP %d/%d    COINS %d"), Run->GetHealth(), Run->GetMaxHealth(), Run->GetCoins()), Cards);
	MapView->SetFooter(TEXT("A / D  CHOOSE    SPACE  TAKE"));
	SetViewTarget(MapView);
}

void AIJPRunGameMode::ShowArena()
{
	SetViewTarget(GetArena());
}

void AIJPRunGameMode::SetViewTarget(AActor* Target) const
{
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (APlayerController* PlayerController = It->Get())
		{
			PlayerController->SetViewTarget(Target);
		}
	}
}
