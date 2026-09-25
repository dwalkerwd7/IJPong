// It's Just Pong

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Audio/IJPToneSet.h"
#include "Audio/IJPToneSynthComponent.h"
#include "Core/IJPTestGameMode.h"
#include "Core/IJPTypes.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "Gameplay/IJPArena.h"
#include "Gameplay/IJPBall.h"
#include "Gameplay/IJPMatchComponent.h"
#include "Gameplay/IJPMatchRules.h"
#include "Gameplay/IJPPaddle.h"
#include "Gameplay/IJPRival.h"
#include "Narrative/IJPBanterComponent.h"
#include "Narrative/IJPConversation.h"
#include "Narrative/IJPSpeechBubbleComponent.h"
#include "Tests/IJPTestWorld.h"

namespace IJPBanterTests
{
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter;

	UIJPConversation* Line(const TCHAR* Text)
	{
		UIJPConversation* Conversation = NewObject<UIJPConversation>(GetTransientPackage());
		FIJPConversationLine& Added = Conversation->Lines.AddDefaulted_GetRef();
		Added.Speaker = EIJPSpeaker::Opponent;
		Added.Text = FText::FromString(Text);
		Added.HoldTime = 0.3f;
		return Conversation;
	}

	void AddBanter(UIJPRival* Rival, EIJPBanterEvent Event, const TCHAR* Text)
	{
		FIJPBanterLines& Lines = Rival->Banter.AddDefaulted_GetRef();
		Lines.Event = Event;
		Lines.Conversations.Add(Line(Text));
		Lines.Chance = 1.f;
	}

	bool RunUntil(FIJPTestWorld& Test, float MaxSeconds, TFunctionRef<bool()> Condition, TFunctionRef<void()> BeforeEachStep = [] {})
	{
		const int32 Steps = FMath::CeilToInt(MaxSeconds / FIJPTestWorld::FixedStep);
		for (int32 i = 0; i < Steps && !Condition(); ++i)
		{
			BeforeEachStep();
			Test.Step();
		}
		return Condition();
	}

	/** Paddle held at the top, ball straight into Side's goal. */
	bool ScoreAgainst(FIJPTestWorld& Test, AIJPArena* Arena, EIJPSide Side)
	{
		AIJPPaddle* Paddle = Arena->GetPaddle(Side);
		const auto HoldUp = [Paddle] { Paddle->AddMoveInput(1.f); };
		Test.RunFor(0.5f, HoldUp);
		AIJPBall* Ball = Arena->GetBall();
		Ball->Serve(Side, 0.f);
		return RunUntil(Test, 3.f, [Ball] { return !Ball->IsInPlay(); }, HoldUp);
	}

	FString RivalLine(const AIJPArena* Arena)
	{
		const UIJPSpeechBubbleComponent* Bubble = Arena->GetPaddle(EIJPSide::Right)->GetSpeechBubble();
		return Bubble->IsTalking() ? Bubble->GetLine() : FString();
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPBanterGoalsTest, "IJPong.Narrative.RivalBantersAboutGoals", IJPBanterTests::Flags)
bool FIJPBanterGoalsTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test;
	AIJPTestGameMode* Mode = Cast<AIJPTestGameMode>(Test.GetWorld()->GetAuthGameMode());
	AIJPArena* Arena = Mode->GetArena();
	UIJPRival* Rival = NewObject<UIJPRival>(GetTransientPackage());
	Rival->BanterCooldown = 0.f;
	Rival->VoicePitch = 0.5f;
	IJPBanterTests::AddBanter(Rival, EIJPBanterEvent::RivalScoredFirst, TEXT("FIRST"));
	IJPBanterTests::AddBanter(Rival, EIJPBanterEvent::RivalScored, TEXT("AGAIN"));
	Mode->SetRival(Rival);
	UTEST_EQUAL("Rival's voice", Arena->GetPaddle(EIJPSide::Right)->GetSpeechBubble()->GetVoicePitch(), 0.5f);

	UTEST_TRUE("Rival scores", IJPBanterTests::ScoreAgainst(Test, Arena, EIJPSide::Left));
	UTEST_EQUAL("First-goal line", IJPBanterTests::RivalLine(Arena), FString(TEXT("FIRST")));
	UTEST_TRUE("Rival scores again", IJPBanterTests::ScoreAgainst(Test, Arena, EIJPSide::Left));
	UTEST_EQUAL("Any-goal line", IJPBanterTests::RivalLine(Arena), FString(TEXT("AGAIN")));
	UTEST_EQUAL("Two reactions", Mode->GetBanter()->GetBanterCount(), 2);
	UTEST_TRUE("Play went on", Mode->GetMatch() && !Mode->GetMatch()->IsServeHeld());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPBanterMatchPointTest, "IJPong.Narrative.MatchPointOutranksGoalBanter", IJPBanterTests::Flags)
bool FIJPBanterMatchPointTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test;
	AIJPTestGameMode* Mode = Cast<AIJPTestGameMode>(Test.GetWorld()->GetAuthGameMode());
	AIJPArena* Arena = Mode->GetArena();
	UIJPRival* Rival = NewObject<UIJPRival>(GetTransientPackage());
	Rival->BanterCooldown = 0.f;
	IJPBanterTests::AddBanter(Rival, EIJPBanterEvent::PlayerScoredFirst, TEXT("LUCKY"));
	IJPBanterTests::AddBanter(Rival, EIJPBanterEvent::PlayerMatchPoint, TEXT("CAREFUL"));
	Mode->SetRival(Rival);
	UIJPMatchRules* FirstToTwo = NewObject<UIJPMatchRules>(GetTransientPackage());
	FirstToTwo->WinTarget = 2;
	Mode->RestartMatch(FirstToTwo);

	// The player scores once: first goal AND match point; match point wins.
	Arena->GetPaddle(EIJPSide::Right)->GetController()->UnPossess();
	UTEST_TRUE("Player scores", IJPBanterTests::ScoreAgainst(Test, Arena, EIJPSide::Right));
	UTEST_EQUAL("Match-point line", IJPBanterTests::RivalLine(Arena), FString(TEXT("CAREFUL")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPBanterRallyTest, "IJPong.Narrative.LongRallyBanterAndCooldown", IJPBanterTests::Flags)
bool FIJPBanterRallyTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test;
	AIJPTestGameMode* Mode = Cast<AIJPTestGameMode>(Test.GetWorld()->GetAuthGameMode());
	AIJPArena* Arena = Mode->GetArena();
	UIJPRival* Rival = NewObject<UIJPRival>(GetTransientPackage());
	Rival->BanterCooldown = 100.f;
	Rival->LongRallyReturns = 2;
	IJPBanterTests::AddBanter(Rival, EIJPBanterEvent::LongRally, TEXT("RALLY"));
	IJPBanterTests::AddBanter(Rival, EIJPBanterEvent::RivalScored, TEXT("GOAL"));
	Mode->SetRival(Rival);

	// Both paddles still in the middle: a straight ball goes back and forth.
	Arena->GetPaddle(EIJPSide::Right)->GetController()->UnPossess();
	Test.RunFor(1.1f);
	AIJPBall* Ball = Arena->GetBall();
	Ball->Serve(EIJPSide::Left, 0.f);
	UTEST_TRUE("Two returns", IJPBanterTests::RunUntil(Test, 4.f, [Ball] { return Ball->GetRallyHits() >= 2; }));
	UTEST_EQUAL("Long-rally line", IJPBanterTests::RivalLine(Arena), FString(TEXT("RALLY")));

	// A goal inside the cooldown passes silently.
	UTEST_TRUE("Rival scores", IJPBanterTests::ScoreAgainst(Test, Arena, EIJPSide::Left));
	UTEST_EQUAL("Only one reaction", Mode->GetBanter()->GetBanterCount(), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPBabbleTest, "IJPong.Narrative.BubblesBabbleInTheirVoice", IJPBanterTests::Flags)
bool FIJPBabbleTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test;
	AIJPArena* Arena = Test.GetArena();
	UIJPToneSynthComponent* Tones = Arena->GetTones();
	UIJPSpeechBubbleComponent* Bubble = Arena->GetPaddle(EIJPSide::Left)->GetSpeechBubble();

	auto PitchOf = [&](const TCHAR* Text, float Voice)
	{
		Bubble->SetVoicePitch(Voice);
		Bubble->Say(FText::FromString(Text), 0.1f);
		Test.RunFor(0.2f);
		return Tones->GetLastTone().Frequency;
	};
	const float B = PitchOf(TEXT("B"), 1.f);
	const float E = PitchOf(TEXT("E"), 1.f);
	const float LowB = PitchOf(TEXT("B"), 0.5f);
	UTEST_TRUE("Letters babble at different pitches", !FMath::IsNearlyEqual(B, E));
	UTEST_EQUAL_TOLERANCE("A lower voice is lower", LowB, B * 0.5f, 0.01f);
	UTEST_EQUAL("Same letter, same pitch", PitchOf(TEXT("B"), 1.f), B);

	const int32 Before = Tones->GetToneCount();
	Bubble->Say(FText::FromString(TEXT("... !")), 0.1f);
	Test.RunFor(0.3f);
	UTEST_EQUAL("Punctuation is silent", Tones->GetToneCount(), Before);
	return true;
}

#endif
