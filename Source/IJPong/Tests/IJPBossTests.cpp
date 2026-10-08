// It's Just Pong

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Abilities/IJPAbility_Bumpers.h"
#include "Abilities/IJPAbility_Spell.h"
#include "Abilities/IJPAbilityComponent.h"
#include "Core/IJPTestGameMode.h"
#include "Core/IJPTypes.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "Era/IJPEra.h"
#include "Era/IJPEraSubsystem.h"
#include "GameFramework/Controller.h"
#include "Gameplay/IJPArena.h"
#include "Gameplay/IJPBall.h"
#include "Gameplay/IJPBossComponent.h"
#include "Gameplay/IJPMatchComponent.h"
#include "Gameplay/IJPPaddle.h"
#include "Gameplay/IJPRival.h"
#include "Narrative/IJPConversation.h"
#include "Presentation/IJPScreenShakeComponent.h"
#include "Narrative/IJPSpeechBubbleComponent.h"
#include "Tests/IJPTestWorld.h"

namespace IJPBossTests
{
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter;

	bool RunUntil(FIJPTestWorld& Test, float MaxSeconds, TFunctionRef<bool()> Condition)
	{
		const int32 Steps = FMath::CeilToInt(MaxSeconds / FIJPTestWorld::FixedStep);
		for (int32 i = 0; i < Steps && !Condition(); ++i)
		{
			Test.Step();
		}
		return Condition();
	}

	UIJPConversation* Line(const TCHAR* Text)
	{
		UIJPConversation* Conversation = NewObject<UIJPConversation>(GetTransientPackage());
		FIJPConversationLine& Added = Conversation->Lines.AddDefaulted_GetRef();
		Added.Speaker = EIJPSpeaker::Opponent;
		Added.Text = FText::FromString(Text);
		Added.HoldTime = 0.5f;
		return Conversation;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPSplitPaddleTest, "IJPong.Boss.SplitPaddleLetsTheBallThroughTheGap", IJPBossTests::Flags)
bool FIJPSplitPaddleTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test;
	AIJPArena* Arena = Test.GetArena();
	AIJPPaddle* Right = Arena->GetPaddle(EIJPSide::Right);
	Right->GetController()->UnPossess(); // stays centred
	Right->SetSplitGap(40.f);
	UTEST_EQUAL("Split", Right->GetSplitGap(), 40.f);
	AIJPBall* Ball = Arena->GetBall();

	// Straight down the middle: through the gap and in.
	Ball->Serve(EIJPSide::Right, 0.f);
	UTEST_TRUE("Through the gap", IJPBossTests::RunUntil(Test, 2.f, [Ball] { return !Ball->IsInPlay(); }));

	// At a half: returned, off that half (not off the gap-centred whole paddle).
	const float HalfOffset = Right->GetSplitHalfOffset();
	Ball->Launch(FVector2D(0.f, HalfOffset), FVector2D(400.f, 0.f));
	UTEST_TRUE("Returned by the top half", IJPBossTests::RunUntil(Test, 2.f, [Ball] { return Ball->GetPlaneVelocity().X < 0.f; }));
	UTEST_TRUE("Straight back: it hit that half's middle", FMath::Abs(Ball->GetPlaneVelocity().Y) < 40.f);

	Right->SetSplitGap(0.f);
	Ball->Serve(EIJPSide::Right, 0.f);
	UTEST_TRUE("Whole again: the middle blocks", IJPBossTests::RunUntil(Test, 2.f, [Ball] { return Ball->GetPlaneVelocity().X < 0.f; }));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPBossPhasesTest, "IJPong.Boss.ShrinksAndChangesAtHealthThresholds", IJPBossTests::Flags)
bool FIJPBossPhasesTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test;
	AIJPTestGameMode* Mode = Cast<AIJPTestGameMode>(Test.GetWorld()->GetAuthGameMode());
	AIJPArena* Arena = Mode->GetArena();
	UIJPMatchComponent* Match = Mode->GetMatch();
	AIJPPaddle* BossPaddle = Arena->GetPaddle(EIJPSide::Right);
	const float NormalLength = BossPaddle->GetSize().Y;

	UIJPAbility_Spell* Brickfall = NewObject<UIJPAbility_Spell>(GetTransientPackage());
	Brickfall->ChargeCost = 4;
	Brickfall->Strikes = 3;
	UIJPRival* Boss = NewObject<UIJPRival>(GetTransientPackage());
	Boss->BossLength = 3.f;
	FIJPBossPhase& Second = Boss->Phases.AddDefaulted_GetRef();
	Second.AtHealth = 0.66f;
	Second.Spell = Brickfall;
	Second.Line = IJPBossTests::Line(TEXT("CRACKS"));
	FIJPBossPhase& Third = Boss->Phases.AddDefaulted_GetRef();
	Third.AtHealth = 0.33f;
	Third.SplitGap = 40.f;
	Mode->SetRival(Boss);
	Mode->RestartMatch();

	UTEST_EQUAL_TOLERANCE("Huge at full health", static_cast<float>(BossPaddle->GetSize().Y), NormalLength * 3.f, 0.1f);
	UTEST_EQUAL("First form", Mode->GetBoss()->GetPhasesEntered(), 0);

	// Down to 3/5: shrinks, and phase two: its spell, ready at once, and its line.
	Match->ApplyDamage(EIJPSide::Right, 2.f);
	UTEST_EQUAL_TOLERANCE("Shrank with its health", static_cast<float>(BossPaddle->GetSize().Y), NormalLength * (1.f + 2.f * 0.6f), 0.1f);
	UTEST_EQUAL("Phase two", Mode->GetBoss()->GetPhasesEntered(), 1);
	UTEST_TRUE("Its spell", BossPaddle->GetAbilities()->GetAbility(EIJPAbilitySlot::Spell) != nullptr);
	UTEST_TRUE("Ready at once", BossPaddle->GetAbilities()->IsReady(EIJPAbilitySlot::Spell));
	Test.Step();
	UTEST_EQUAL("Says so", BossPaddle->GetSpeechBubble()->GetLine(), FString(TEXT("CRACKS")));
	UTEST_EQUAL("Not split yet", BossPaddle->GetSplitGap(), 0.f);

	// Down to 1/5: crumbles in two.
	Match->ApplyDamage(EIJPSide::Right, 2.f);
	UTEST_EQUAL("Phase three", Mode->GetBoss()->GetPhasesEntered(), 2);
	UTEST_EQUAL("In two", BossPaddle->GetSplitGap(), 40.f);

	// A new match starts it over.
	Mode->RestartMatch();
	UTEST_EQUAL("First form again", Mode->GetBoss()->GetPhasesEntered(), 0);
	UTEST_EQUAL("Whole again", BossPaddle->GetSplitGap(), 0.f);
	UTEST_TRUE("No spell again", BossPaddle->GetAbilities()->GetAbility(EIJPAbilitySlot::Spell) == nullptr);
	UTEST_EQUAL_TOLERANCE("Huge again", static_cast<float>(BossPaddle->GetSize().Y), NormalLength * 3.f, 0.1f);

	// An ordinary rival: normal size.
	Mode->SetRival(NewObject<UIJPRival>(GetTransientPackage()));
	UTEST_EQUAL_TOLERANCE("Back to normal", static_cast<float>(BossPaddle->GetSize().Y), NormalLength, 0.1f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPBrickfallTest, "IJPong.Spell.SeveralStrikesAtOnce", IJPBossTests::Flags)
bool FIJPBrickfallTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test;
	AIJPArena* Arena = Test.GetArena();
	UIJPAbilityComponent* Abilities = Arena->GetPaddle(EIJPSide::Left)->GetAbilities();
	UIJPAbility_Spell* Brickfall = NewObject<UIJPAbility_Spell>(GetTransientPackage());
	Brickfall->ChargeCost = 1;
	Brickfall->Strikes = 3;
	Brickfall->StrikeSpacing = 140.f;
	Abilities->Equip(EIJPAbilitySlot::Spell, Brickfall);
	Abilities->AddCharge(1);
	UTEST_TRUE("Cast", Abilities->TryActivate(EIJPAbilitySlot::Spell));
	UTEST_EQUAL("Three at once", Arena->GetStrikes().Num(), 3);
	float Lowest = 1e6f, Highest = -1e6f;
	for (const TWeakObjectPtr<AIJPSpellStrike>& Strike : Arena->GetStrikes())
	{
		Lowest = FMath::Min(Lowest, Strike->GetTargetY());
		Highest = FMath::Max(Highest, Strike->GetTargetY());
	}
	UTEST_EQUAL_TOLERANCE("Spread along the lane", Highest - Lowest, 280.f, 0.5f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPTiltBossTest, "IJPong.Boss.TiltBumpersMultiBallThenTilt", IJPBossTests::Flags)
bool FIJPTiltBossTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test;
	AIJPTestGameMode* Mode = Cast<AIJPTestGameMode>(Test.GetWorld()->GetAuthGameMode());
	AIJPArena* Arena = Mode->GetArena();
	UIJPMatchComponent* Match = Mode->GetMatch();
	AIJPPaddle* BossPaddle = Arena->GetPaddle(EIJPSide::Right);

	UIJPRival* Tilt = NewObject<UIJPRival>(GetTransientPackage());
	Tilt->BossLength = 2.f;
	Tilt->RivalSkill = NewObject<UIJPAbility_Bumpers>(GetTransientPackage());
	FIJPBossPhase& Multi = Tilt->Phases.AddDefaulted_GetRef();
	Multi.AtHealth = 0.66f;
	Multi.Pause = 0.f;
	Multi.LaunchBalls = 2;
	FIJPBossPhase& TiltPhase = Tilt->Phases.AddDefaulted_GetRef();
	TiltPhase.AtHealth = 0.33f;
	TiltPhase.Pause = 0.f;
	TiltPhase.BallSpeedScale = 1.5f;
	TiltPhase.ScreenShake = 1.f;
	Mode->SetRival(Tilt);
	Mode->RestartMatch();

	// Its skill: bumpers as soon as a ball is in play.
	Test.RunFor(1.2f);
	const UIJPAbility_Bumpers* Bumpers = Cast<UIJPAbility_Bumpers>(BossPaddle->GetAbilities()->GetAbility(EIJPAbilitySlot::ClassSkill));
	UTEST_NOT_NULL("Tilt's skill", Bumpers);
	UTEST_TRUE("Bumpers down", Bumpers->GetNumBumpers() > 0);

	// Phase two: multi-ball.
	const int32 BallsBefore = Arena->GetNumBallsInPlay();
	Match->ApplyDamage(EIJPSide::Right, 2.f);
	UTEST_EQUAL("Two more balls", Arena->GetNumBallsInPlay(), BallsBefore + 2);

	// Phase three: TILT.
	Match->ApplyDamage(EIJPSide::Right, 2.f);
	UTEST_EQUAL_TOLERANCE("Everything faster", Arena->GetBallSpeedScale(), 1.5f, 0.001f);
	Test.Step();
	UTEST_TRUE("The screen shakes", Arena->GetScreenShake()->IsShaking());

	Mode->RestartMatch();
	UTEST_EQUAL_TOLERANCE("A new match, normal speed", Arena->GetBallSpeedScale(), 1.f, 0.001f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPBossSpritesTest, "IJPong.Boss.SpritesCrackThenBreakInTwo", IJPBossTests::Flags)
bool FIJPBossSpritesTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test;
	AIJPTestGameMode* Mode = Cast<AIJPTestGameMode>(Test.GetWorld()->GetAuthGameMode());
	AIJPArena* Arena = Mode->GetArena();
	UIJPMatchComponent* Match = Mode->GetMatch();
	AIJPPaddle* BossPaddle = Arena->GetPaddle(EIJPSide::Right);
	UIJPEra* Era = NewObject<UIJPEra>(GetTransientPackage());
	Era->bShowSprites = true;
	UIJPEraSubsystem::Get(Arena)->SetEra(Era);

	UTexture2D* Whole = UTexture2D::CreateTransient(8, 64);
	UTexture2D* Cracked = UTexture2D::CreateTransient(8, 64);
	UTexture2D* Half = UTexture2D::CreateTransient(8, 32);
	UIJPRival* Boss = NewObject<UIJPRival>(GetTransientPackage());
	Boss->BossLength = 3.f;
	Boss->BossSprite = Whole;
	Boss->BossHalfSprite = Half;
	FIJPBossPhase& Second = Boss->Phases.AddDefaulted_GetRef();
	Second.AtHealth = 0.66f;
	Second.Pause = 0.f;
	Second.Sprite = Cracked;
	FIJPBossPhase& Third = Boss->Phases.AddDefaulted_GetRef();
	Third.AtHealth = 0.33f;
	Third.Pause = 0.f;
	Third.SplitGap = 40.f;
	Mode->SetRival(Boss);
	Mode->RestartMatch();

	UTEST_TRUE("Its own paddle", BossPaddle->GetShownSprite() == Whole);
	Match->ApplyDamage(EIJPSide::Right, 2.f);
	UTEST_TRUE("Cracked in phase two", BossPaddle->GetShownSprite() == Cracked);
	Match->ApplyDamage(EIJPSide::Right, 2.f);
	UTEST_TRUE("Two halves in phase three", BossPaddle->GetShownSprite() == Half);
	UTEST_TRUE("Drawn as sprites, not boxes", BossPaddle->IsSpriteShown());

	// A new match: whole again, and an ordinary rival gets its class's look back.
	Mode->RestartMatch();
	UTEST_TRUE("Whole again", BossPaddle->GetShownSprite() == Whole);
	Mode->SetRival(nullptr);
	UTEST_FALSE("No boss art on an ordinary paddle", BossPaddle->GetShownSprite() == Whole);
	return true;
}

#endif
