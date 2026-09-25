// It's Just Pong

#include "Core/IJPTestGameMode.h"
#include "Abilities/IJPAbilityComponent.h"
#include "AI/IJPPaddleAIController.h"
#include "Core/IJPTestHUD.h"
#include "Core/IJPTestPlayerController.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "Era/IJPEra.h"
#include "Era/IJPEraSubsystem.h"
#include "Gameplay/IJPArena.h"
#include "Gameplay/IJPBall.h"
#include "Gameplay/IJPBallType.h"
#include "Gameplay/IJPMatchComponent.h"
#include "Gameplay/IJPMatchRules.h"
#include "Gameplay/IJPPaddle.h"
#include "Gameplay/IJPPaddleClass.h"
#include "Gameplay/IJPRival.h"

AIJPTestGameMode::AIJPTestGameMode()
{
	PlayerControllerClass = AIJPTestPlayerController::StaticClass();
	HUDClass = AIJPTestHUD::StaticClass();
}

void AIJPTestGameMode::OnArenaReady()
{
	if (AIJPPaddle* PlayerPaddle = GetArena()->GetPaddle(PlayerSide))
	{
		PlayerPaddle->GetAbilities()->Equip(EIJPAbilitySlot::RunAbility, PlayerRunAbility.LoadSynchronous());
		PlayerPaddle->GetAbilities()->Equip(EIJPAbilitySlot::Spell, PlayerSpell.LoadSynchronous());
	}
	if (Rivals.IsValidIndex(StartingRival))
	{
		RivalIndex = StartingRival;
		SetRival(Rivals[RivalIndex].LoadSynchronous());
	}
	RestartMatch();
}

void AIJPTestGameMode::RestartMatch(const UIJPMatchRules* Rules)
{
	if (AIJPPaddle* PlayerPaddle = GetArena() ? GetArena()->GetPaddle(PlayerSide) : nullptr)
	{
		PlayerPaddle->GetAbilities()->Equip(EIJPAbilitySlot::Item, PlayerItem.LoadSynchronous());
	}
	BeginMatch(Rules ? Rules : MatchRules.LoadSynchronous());
}

void AIJPTestGameMode::ServeNow()
{
	if (GetMatch()->IsOver())
	{
		RestartMatch();
	}
	else
	{
		GetMatch()->ServeNow();
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

void AIJPTestGameMode::AdjustOpponentSkill(float Delta)
{
	AIJPArena* ArenaPtr = GetArena();
	if (!ArenaPtr)
	{
		return;
	}

	ArenaPtr->SetOpponentSkill(ArenaPtr->GetOpponentSkill() + Delta);
	const float Skill = ArenaPtr->GetOpponentSkill();
	for (TActorIterator<AIJPPaddleAIController> It(GetWorld()); It; ++It)
	{
		It->SetSkill(Skill);
	}

	UE_LOG(LogIJPong, Log, TEXT("Opponent skill: %.2f"), Skill);
	if (GEngine)
	{
		// Same key each time, so repeated presses replace the message instead of stacking.
		GEngine->AddOnScreenDebugMessage(static_cast<uint64>(GetUniqueID()), 2.f, FColor::White, FString::Printf(TEXT("Opponent skill: %.2f"), Skill));
	}
}

void AIJPTestGameMode::CycleEra(int32 Direction)
{
	UIJPEraSubsystem* Eras = UIJPEraSubsystem::Get(this);
	if (!Eras || Eras->GetNumEras() == 0)
	{
		return;
	}

	// From an era that isn't in the list (set by code), start counting from the first.
	const int32 Current = FMath::Max(Eras->GetEraIndex(), 0);
	Eras->SetEraIndex((Current + Direction % Eras->GetNumEras() + Eras->GetNumEras()) % Eras->GetNumEras());

	const FString Name = Eras->GetEra()->DisplayName.ToString();
	UE_LOG(LogIJPong, Log, TEXT("Era: %s"), *Name);
	if (GEngine)
	{
		// Same key each time, so repeated presses replace the message instead of stacking.
		GEngine->AddOnScreenDebugMessage(static_cast<uint64>(GetUniqueID()) + 1, 2.f, FColor::White, FString::Printf(TEXT("Era: %s"), *Name));
	}
}

void AIJPTestGameMode::CyclePlayerClass(int32 Direction)
{
	AIJPPaddle* Paddle = GetArena() ? GetArena()->GetPaddle(PlayerSide) : nullptr;
	if (!Paddle || PlayerClasses.IsEmpty())
	{
		return;
	}

	// From a class that isn't in the list, start counting from the first.
	const int32 Num = PlayerClasses.Num();
	const int32 Current = FMath::Max(PlayerClasses.IndexOfByKey(Paddle->GetPaddleClass()), 0);
	const UIJPPaddleClass* Next = PlayerClasses[((Current + Direction) % Num + Num) % Num].LoadSynchronous();
	Paddle->SetPaddleClass(Next);
	ShowMessage(2, FString::Printf(TEXT("Your class: %s"), Next ? *Next->DisplayName.ToString() : TEXT("none")));
}

void AIJPTestGameMode::CycleRival(int32 Direction)
{
	// Positions -1 (no rival) .. Num-1, wrapping.
	const int32 Count = Rivals.Num() + 1;
	RivalIndex = ((RivalIndex + 1 + Direction) % Count + Count) % Count - 1;
	const UIJPRival* Next = Rivals.IsValidIndex(RivalIndex) ? Rivals[RivalIndex].LoadSynchronous() : nullptr;
	SetRival(Next);
	// A fresh match, so the new rival gets to say hello.
	RestartMatch();
	ShowMessage(3, FString::Printf(TEXT("Opponent: %s"), Next ? *Next->DisplayName.ToString() : TEXT("no rival")));
}

void AIJPTestGameMode::ShowMessage(int32 Key, const FString& Message) const
{
	UE_LOG(LogIJPong, Log, TEXT("%s"), *Message);
	if (GEngine)
	{
		// Same key each time, so repeated presses replace the message instead of stacking.
		GEngine->AddOnScreenDebugMessage(static_cast<uint64>(GetUniqueID()) + Key, 2.f, FColor::White, Message);
	}
}

void AIJPTestGameMode::AddRandomBall()
{
	const UIJPBallType* Type = ExtraBallTypes.IsEmpty() ? nullptr : ExtraBallTypes[FMath::RandHelper(ExtraBallTypes.Num())].LoadSynchronous();
	if (AIJPBall* Extra = GetMatch()->LaunchExtraBall(Type))
	{
		UE_LOG(LogIJPong, Log, TEXT("Extra ball: %s"), *Extra->GetType().DisplayName.ToString());
	}
}

void AIJPTestGameMode::GetDebugLines(TArray<FString>& OutLines) const
{
	const AIJPArena* ArenaPtr = GetArena();
	OutLines.Add(TEXT("TEST MODE"));
	OutLines.Add(FString::Printf(TEXT("Opponent skill: %.2f"), ArenaPtr ? ArenaPtr->GetOpponentSkill() : 0.f));
	OutLines.Add(FString::Printf(TEXT("Your paddle: %s"), IsPlayerSideAI() ? TEXT("AI") : TEXT("you")));

	const UIJPEraSubsystem* Eras = UIJPEraSubsystem::Get(this);
	const UIJPEra* Era = Eras ? Eras->GetEra() : nullptr;
	OutLines.Add(Era
		? FString::Printf(TEXT("Era: %s (%d/%d)"), *Era->DisplayName.ToString(), Eras->GetEraIndex() + 1, Eras->GetNumEras())
		: FString(TEXT("Era: none")));

	OutLines.Add(FString::Printf(TEXT("Balls in play: %d"), ArenaPtr ? ArenaPtr->GetNumBallsInPlay() : 0));
	{
		const AIJPPaddle* PlayerPaddle = ArenaPtr ? ArenaPtr->GetPaddle(PlayerSide) : nullptr;
		const UIJPAbilityComponent* PlayerAbilities = PlayerPaddle ? PlayerPaddle->GetAbilities() : nullptr;
		const UIJPAbility* Item = PlayerAbilities ? PlayerAbilities->GetAbility(EIJPAbilitySlot::Item) : nullptr;
		OutLines.Add(FString::Printf(TEXT("Item (C): %s"), Item && PlayerAbilities->HasItem() ? *Item->DisplayName.ToString() : TEXT("none (R refills)")));
	}

	auto ClassName = [ArenaPtr](EIJPSide Side) -> FString
	{
		const AIJPPaddle* Paddle = ArenaPtr ? ArenaPtr->GetPaddle(Side) : nullptr;
		const UIJPPaddleClass* PaddleClass = Paddle ? Paddle->GetPaddleClass() : nullptr;
		return PaddleClass ? PaddleClass->DisplayName.ToString() : FString(TEXT("none"));
	};
	const UIJPRival* CurrentRival = GetRival();
	OutLines.Add(FString::Printf(TEXT("You: %s   Opponent: %s (%s)"), *ClassName(PlayerSide),
		CurrentRival ? *CurrentRival->DisplayName.ToString() : TEXT("no rival"), *ClassName(IJP::Opposite(PlayerSide))));

	// The player's abilities: name and state per slot.
	const AIJPPaddle* PlayerPaddle = ArenaPtr ? ArenaPtr->GetPaddle(PlayerSide) : nullptr;
	if (const UIJPAbilityComponent* Abilities = PlayerPaddle ? PlayerPaddle->GetAbilities() : nullptr)
	{
		auto Describe = [Abilities](EIJPAbilitySlot Slot, const TCHAR* Key) -> FString
		{
			const UIJPAbility* Ability = Abilities->GetAbility(Slot);
			if (!Ability)
			{
				return FString::Printf(TEXT("%s: -"), Key);
			}
			const FString State = Ability->IsActive() ? FString(TEXT("ACTIVE"))
				: Abilities->IsReady(Slot) ? FString(TEXT("ready"))
				: FString::Printf(TEXT("%.1fs"), Abilities->GetCooldownRemaining(Slot));
			return FString::Printf(TEXT("%s %s: %s"), Key, *Ability->DisplayName.ToString(), *State);
		};
		OutLines.Add(Describe(EIJPAbilitySlot::ClassSkill, TEXT("[Space]")) + TEXT("   ") + Describe(EIJPAbilitySlot::RunAbility, TEXT("[E]")));
	}

	const UIJPMatchComponent* MatchPtr = GetMatch();
	if (MatchPtr->IsOver())
	{
		OutLines.Add(FString::Printf(TEXT("Match over: %s wins"), MatchPtr->GetWinner() == EIJPSide::Left ? TEXT("left") : TEXT("right")));
	}
	else if (MatchPtr->GetRules().IsEndless())
	{
		OutLines.Add(TEXT("Match: endless"));
	}
	else
	{
		OutLines.Add(FString::Printf(TEXT("Match: health %g each, %g per goal"), MatchPtr->GetRules().StartingHealth, MatchPtr->GetRules().GoalDamage));
	}

	OutLines.Add(TEXT("R new match   F serve now   T AI vs AI   B add ball"));
	OutLines.Add(TEXT("- / = opponent skill   [ / ] era   N class   V rival   . (period) hide this"));
}
