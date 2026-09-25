// It's Just Pong

#include "Core/IJPRunGameMode.h"
#include "Abilities/IJPAbility.h"
#include "Abilities/IJPAbilityComponent.h"
#include "Engine/World.h"
#include "Gameplay/IJPArena.h"
#include "Gameplay/IJPMatchComponent.h"
#include "Gameplay/IJPPaddle.h"
#include "Gameplay/IJPPaddleClass.h"
#include "Gameplay/IJPRival.h"
#include "Meta/IJPMetaSubsystem.h"
#include "Meta/IJPSkillTree.h"
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

	GetMatch()->OnHealthChanged.AddDynamic(this, &AIJPRunGameMode::HandleHealthChanged);
	GetMatch()->OnMatchEnded.AddDynamic(this, &AIJPRunGameMode::HandleRunMatchEnded);

	StartNewRun();
}

void AIJPRunGameMode::StartNewRun(const UIJPActConfig* Act, int32 Seed, float InStartingHealth)
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
	RunStartingHealth = InStartingHealth > 0.f ? InStartingHealth : StartingHealth;
	Run->StartRun(RunAct, Seed == INDEX_NONE ? FMath::Rand() : Seed, RunStartingHealth);
	ApplyLoadout(); // a fresh run: nothing gathered yet
	Phase = EIJPRunPhase::Map;
	ShowMap();
}

bool AIJPRunGameMode::HandleUIStep(int32 Direction)
{
	if (Phase != EIJPRunPhase::Map && Phase != EIJPRunPhase::Reward && Phase != EIJPRunPhase::Tree)
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
		if (const UIJPSkillTree* Tree = GetPlayerTree())
		{
			// Between runs: grow the class first.
			Phase = EIJPRunPhase::Tree;
			MapView->ShowTree(Tree, true);
			MapView->SetFooter(TEXT("A / D  CHOOSE    SPACE  BUY / START"));
			SetViewTarget(MapView);
		}
		else
		{
			StartNewRun(RunAct, INDEX_NONE, RunStartingHealth);
		}
		return true;
	case EIJPRunPhase::Tree:
	{
		const int32 Node = MapView->GetSelectedTreeNode();
		if (Node == INDEX_NONE)
		{
			// START RUN: same act and health as the run that just ended, on a fresh map.
			StartNewRun(RunAct, INDEX_NONE, RunStartingHealth);
		}
		else if (UIJPMetaSubsystem* Meta = UIJPMetaSubsystem::Get(this); Meta && Meta->Buy(GetPlayerTree(), Node))
		{
			MapView->ShowTree(GetPlayerTree(), false);
		}
		return true;
	}
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
	// The rival starts full (the rules' health); the player fights on what's left of the run's.
	GetMatch()->SetHealth(PlayerSide, Run->GetHealth(), Run->GetMaxHealth());
	Phase = EIJPRunPhase::Playing;
	ShowArena();
}

void AIJPRunGameMode::HandleHealthChanged(EIJPSide Side, float Health, float Damage)
{
	if (Phase != EIJPRunPhase::Playing || Side != PlayerSide)
	{
		return;
	}

	// The match started from the run's health, so the same damage keeps the two in step, even in
	// a match the player goes on to win. Running out ends the run, not just the match.
	UIJPRunSubsystem* Run = UIJPRunSubsystem::Get(this);
	Run->LoseHealth(Damage);
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
		// What this run earned for the skill trees, and the totals so far.
		const UIJPMetaSubsystem* Meta = UIJPMetaSubsystem::Get(this);
		MapView->SetHeader(FString::Printf(TEXT("%s    +%d SKILL PTS    +%d BOSS TOKENS"),
			Run->GetState() == EIJPRunState::Won ? TEXT("ACT CLEARED!") : TEXT("RUN OVER"),
			Run->GetEarnedSkillPoints(), Run->GetEarnedBossTokens()));
		MapView->SetFooter(FString::Printf(TEXT("SKILL PTS %d    BOSS TOKENS %d    SPACE: NEW RUN"),
			Meta ? Meta->GetSkillPoints() : 0, Meta ? Meta->GetBossTokens() : 0));
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

	// This run's pickups, plus what the class's skill tree gives every run.
	const FIJPRunLoadout& Loadout = Run->GetLoadout();
	const UIJPMetaSubsystem* Meta = UIJPMetaSubsystem::Get(this);
	const FIJPTreeBonuses Tree = Meta ? Meta->GetBonuses(GetPlayerTree()) : FIJPTreeBonuses();

	Paddle->SetRunScales(1.f + Loadout.PaddleLengthBonus + Tree.PaddleLength, 1.f + Loadout.PaddleSpeedBonus + Tree.PaddleSpeed);
	Paddle->SetReturnAngleBonus(Tree.ReturnAngle);
	UIJPAbilityComponent* Abilities = Paddle->GetAbilities();
	Abilities->SetCooldownScale(EIJPAbilitySlot::ClassSkill, FMath::Max(1.f - Loadout.ClassSkillCooldownCut - Tree.SkillCooldownCut, 0.1f));
	if (UIJPAbility* ClassSkill = Abilities->GetAbility(EIJPAbilitySlot::ClassSkill))
	{
		ClassSkill->SetUpgrades(Tree.SkillUpgrades);
	}
	Abilities->Equip(EIJPAbilitySlot::RunAbility, Loadout.RunAbility);
	GetMatch()->SetExtraServedBalls(Loadout.ExtraServedBalls);
}

const UIJPSkillTree* AIJPRunGameMode::GetPlayerTree() const
{
	const AIJPPaddle* Paddle = GetArena() ? GetArena()->GetPaddle(PlayerSide) : nullptr;
	const UIJPPaddleClass* PaddleClass = Paddle ? Paddle->GetPaddleClass() : nullptr;
	return PaddleClass ? PaddleClass->SkillTree.Get() : nullptr;
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

	MapView->ShowCards(FString::Printf(TEXT("PICK A REWARD    HP %d/%d    COINS %d"), FMath::CeilToInt(Run->GetHealth()), FMath::CeilToInt(Run->GetMaxHealth()), Run->GetCoins()), Cards);
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
