// It's Just Pong

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Core/IJPTestGameMode.h"
#include "Core/IJPTypes.h"
#include "Era/IJPEra.h"
#include "Era/IJPEraSubsystem.h"
#include "Engine/World.h"
#include "Gameplay/IJPArena.h"
#include "Gameplay/IJPBall.h"
#include "Gameplay/IJPMatchComponent.h"
#include "Gameplay/IJPMatchRules.h"
#include "Gameplay/IJPPaddle.h"
#include "Gameplay/IJPRival.h"
#include "Narrative/IJPConversation.h"
#include "Narrative/IJPConversationPlayer.h"
#include "Narrative/IJPSpeechBubbleComponent.h"
#include "Tests/IJPTestWorld.h"

namespace IJPNarrativeTests
{
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter;

	AIJPTestGameMode* GetMode(const FIJPTestWorld& Test)
	{
		return Cast<AIJPTestGameMode>(Test.GetWorld()->GetAuthGameMode());
	}

	UIJPSpeechBubbleComponent* Bubble(const AIJPArena* Arena, EIJPSide Side)
	{
		return Arena->GetPaddle(Side)->GetSpeechBubble();
	}

	UIJPConversation* MakeConversation(std::initializer_list<TPair<EIJPSpeaker, const TCHAR*>> Lines, float HoldTime)
	{
		UIJPConversation* Conversation = NewObject<UIJPConversation>(GetTransientPackage());
		for (const TPair<EIJPSpeaker, const TCHAR*>& Line : Lines)
		{
			FIJPConversationLine& Added = Conversation->Lines.AddDefaulted_GetRef();
			Added.Speaker = Line.Key;
			Added.Text = FText::FromString(Line.Value);
			Added.HoldTime = HoldTime;
		}
		return Conversation;
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
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPBubbleTest, "IJPong.Narrative.BubbleTypesOutHoldsThenHides", IJPNarrativeTests::Flags)
bool FIJPBubbleTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test;
	UIJPSpeechBubbleComponent* Bubble = IJPNarrativeTests::Bubble(Test.GetArena(), EIJPSide::Left);
	UTEST_FALSE("Quiet to start with", Bubble->IsTalking());

	Bubble->Say(FText::FromString(TEXT("HELLO THERE")), 0.5f);
	UTEST_TRUE("Talking", Bubble->IsTalking());
	UTEST_TRUE("Shown", Bubble->IsVisible());
	UTEST_EQUAL("Starts empty", Bubble->GetShownText(), FString());

	Test.RunFor(0.15f);
	const int32 Partway = Bubble->GetShownText().Len();
	UTEST_TRUE("Typing out", Partway > 0 && Partway < 11);

	Test.RunFor(0.3f);
	UTEST_EQUAL("All typed", Bubble->GetShownText(), FString(TEXT("HELLO THERE")));
	UTEST_TRUE("Held up", Bubble->IsTalking());

	Test.RunFor(0.6f);
	UTEST_FALSE("Done", Bubble->IsTalking());
	UTEST_FALSE("Hidden", Bubble->IsVisible());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPConversationTest, "IJPong.Narrative.ConversationTakesTurns", IJPNarrativeTests::Flags)
bool FIJPConversationTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test;
	AIJPTestGameMode* Mode = IJPNarrativeTests::GetMode(Test);
	AIJPArena* Arena = Mode->GetArena();
	UIJPSpeechBubbleComponent* Player = IJPNarrativeTests::Bubble(Arena, EIJPSide::Left);
	UIJPSpeechBubbleComponent* Opponent = IJPNarrativeTests::Bubble(Arena, EIJPSide::Right);

	Mode->PlayConversation(IJPNarrativeTests::MakeConversation({ { EIJPSpeaker::Opponent, TEXT("HI") }, { EIJPSpeaker::Player, TEXT("YO") } }, 0.3f));
	UTEST_TRUE("Opponent speaks first", Opponent->IsTalking() && !Player->IsTalking());
	UTEST_EQUAL("Their line", Opponent->GetLine(), FString(TEXT("HI")));

	UTEST_TRUE("Then the player answers", IJPNarrativeTests::RunUntil(Test, 2.f, [Player] { return Player->IsTalking(); }));
	UTEST_FALSE("One at a time", Opponent->IsTalking());
	UTEST_EQUAL("Player's line", Player->GetLine(), FString(TEXT("YO")));

	UTEST_TRUE("Finishes", IJPNarrativeTests::RunUntil(Test, 2.f, [Mode] { return !Mode->GetConversations()->IsPlaying(); }));
	UTEST_FALSE("Bubbles down", Player->IsTalking() || Opponent->IsTalking());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPPreMatchTest, "IJPong.Narrative.PreMatchConversationHoldsTheServe", IJPNarrativeTests::Flags)
bool FIJPPreMatchTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test;
	AIJPTestGameMode* Mode = IJPNarrativeTests::GetMode(Test);
	AIJPArena* Arena = Mode->GetArena();

	UIJPRival* Rival = NewObject<UIJPRival>(GetTransientPackage());
	Rival->PreMatch.Add(IJPNarrativeTests::MakeConversation({ { EIJPSpeaker::Opponent, TEXT("READY?") } }, 1.f));
	Mode->SetRival(Rival);
	Mode->RestartMatch();

	// The serve delay (1 s) has passed, but the rival is still talking.
	Test.RunFor(1.1f);
	UTEST_TRUE("Rival talking", IJPNarrativeTests::Bubble(Arena, EIJPSide::Right)->IsTalking());
	UTEST_TRUE("Serve held", Mode->GetMatch()->IsServeHeld());
	UTEST_FALSE("No ball yet", Arena->GetBall()->IsInPlay());
	UTEST_TRUE("Ball waits blinking", Arena->GetBall()->IsBlinking());

	UTEST_TRUE("Served once they're done", IJPNarrativeTests::RunUntil(Test, 3.f, [Arena] { return Arena->GetBall()->IsInPlay(); }));
	UTEST_FALSE("Rival quiet by then", IJPNarrativeTests::Bubble(Arena, EIJPSide::Right)->IsTalking());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPResultTest, "IJPong.Narrative.RivalReactsToTheResult", IJPNarrativeTests::Flags)
bool FIJPResultTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test;
	AIJPTestGameMode* Mode = IJPNarrativeTests::GetMode(Test);
	AIJPArena* Arena = Mode->GetArena();
	AIJPPaddle* Left = Arena->GetPaddle(EIJPSide::Left);

	UIJPRival* Rival = NewObject<UIJPRival>(GetTransientPackage());
	Rival->Win.Add(IJPNarrativeTests::MakeConversation({ { EIJPSpeaker::Opponent, TEXT("GG") } }, 1.f));
	Rival->Loss.Add(IJPNarrativeTests::MakeConversation({ { EIJPSpeaker::Opponent, TEXT("NEXT TIME") } }, 1.f));
	Mode->SetRival(Rival);
	UIJPMatchRules* OnePoint = NewObject<UIJPMatchRules>(GetTransientPackage());
	OnePoint->StartingHealth = 1.f;
	Mode->RestartMatch(OnePoint);

	// The player's paddle steps aside and the ball goes straight into their goal: the rival wins.
	Test.RunFor(1.1f, [Left] { Left->AddMoveInput(1.f); });
	Arena->GetBall()->Serve(EIJPSide::Left, 0.f);
	UTEST_TRUE("Match over", IJPNarrativeTests::RunUntil(Test, 3.f, [Mode] { return Mode->GetMatch()->IsOver(); }, [Left] { Left->AddMoveInput(1.f); }));
	UTEST_EQUAL("Rival won", Mode->GetMatch()->GetWinner(), EIJPSide::Right);
	UTEST_TRUE("Rival talking", IJPNarrativeTests::Bubble(Arena, EIJPSide::Right)->IsTalking());
	UTEST_EQUAL("Their winning line", IJPNarrativeTests::Bubble(Arena, EIJPSide::Right)->GetLine(), FString(TEXT("GG")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPBubbleEraStyleTest, "IJPong.Narrative.BubbleShapeFollowsTheEra", IJPNarrativeTests::Flags)
bool FIJPBubbleEraStyleTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test;
	AIJPArena* Arena = Test.GetArena();
	UIJPEraSubsystem* Eras = UIJPEraSubsystem::Get(Arena);
	UIJPSpeechBubbleComponent* Bubble = IJPNarrativeTests::Bubble(Arena, EIJPSide::Left);

	// 1972: square box, pixel-stepped tail.
	Bubble->Say(FText::FromString(TEXT("HI")));
	UTEST_EQUAL("Cabinet: square corners", Bubble->GetCornerRadius(), 0.f);
	UTEST_TRUE("Cabinet: stepped tail", Bubble->IsTailStepped());

	// A later era rounds the corners and smooths the tail, from the next line on.
	UIJPEra* Later = NewObject<UIJPEra>(GetTransientPackage());
	Later->BubbleCornerRadius = 8.f;
	Later->bSmoothBubbleTail = true;
	Eras->SetEra(Later);
	Bubble->Say(FText::FromString(TEXT("HELLO")));
	UTEST_EQUAL("Later era: rounded corners", Bubble->GetCornerRadius(), 8.f);
	UTEST_FALSE("Later era: smooth tail", Bubble->IsTailStepped());
	Eras->SetEraIndex(0);
	return true;
}

#endif
