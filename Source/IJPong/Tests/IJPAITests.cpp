// It's Just Pong

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "AI/IJPAIProfile.h"
#include "AI/IJPPaddleAIController.h"
#include "Core/IJPTestGameMode.h"
#include "Core/IJPTypes.h"
#include "Engine/World.h"
#include "Gameplay/IJPArena.h"
#include "Gameplay/IJPBall.h"
#include "Gameplay/IJPMatchComponent.h"
#include "Gameplay/IJPMatchRules.h"
#include "Gameplay/IJPPaddle.h"
#include "Tests/IJPTestWorld.h"

namespace IJPAITests
{
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter;

	const FTransform ArenaTransform(FRotator(0.f, 45.f, 0.f), FVector(100.f, 100.f, -250.f));

	AIJPPaddleAIController* GetAI(const AIJPArena* Arena, EIJPSide Side)
	{
		return Cast<AIJPPaddleAIController>(Arena->GetPaddle(Side)->GetController());
	}

	/** Reads the ball perfectly and always hits dead centre: isolates the mechanics from the randomness. */
	UIJPAIProfile* MakePerfectProfile()
	{
		UIJPAIProfile* Profile = NewObject<UIJPAIProfile>();
		Profile->ErrorSpread = { 0.f, 0.f };
		Profile->AimSpread = { 0.f, 0.f };
		return Profile;
	}

	/** Long AI-vs-AI runs count goals over minutes, so nobody may win and stop the match. */
	UIJPMatchRules* EndlessRules()
	{
		UIJPMatchRules* Rules = NewObject<UIJPMatchRules>();
		Rules->StartingHealth = 0.f;
		return Rules;
	}

	bool RunUntil(FIJPTestWorld& Test, float MaxSeconds, TFunctionRef<bool()> Condition)
	{
		const int32 Steps = FMath::CeilToInt(MaxSeconds / FIJPTestWorld::FixedStep);
		for (int32 i = 0; i < Steps && !Condition(); ++i)
		{
			Test.Step();
		}
		return Condition();
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPAIPossessTest, "IJPong.AI.PlaysTheRightPaddle", IJPAITests::Flags)
bool FIJPAIPossessTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test(IJPAITests::ArenaTransform);
	AIJPArena* Arena = Test.GetArena();

	UTEST_NOT_NULL("Right paddle has an AI controller", IJPAITests::GetAI(Arena, EIJPSide::Right));
	UTEST_NULL("Left paddle is left for the player", Arena->GetPaddle(EIJPSide::Left)->GetController());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPAIReturnsTest, "IJPong.AI.PerfectProfileReturnsEveryAngle", IJPAITests::Flags)
bool FIJPAIReturnsTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test(IJPAITests::ArenaTransform);
	AIJPArena* Arena = Test.GetArena();
	AIJPBall* Ball = Arena->GetBall();
	AIJPPaddleAIController* AI = IJPAITests::GetAI(Arena, EIJPSide::Right);
	UTEST_NOT_NULL("AI", AI);
	AI->SetProfile(IJPAITests::MakePerfectProfile());

	// Each serve starts wherever the last return left the paddle, so this also covers re-positioning.
	// (Serving again straight away also means the game mode's own first serve is always skipped.)
	for (const float Angle : { 0.f, 45.f, -45.f, 20.f, -60.f, 60.f })
	{
		Ball->Serve(EIJPSide::Right, Angle);
		const bool bDone = IJPAITests::RunUntil(Test, 3.f, [Ball] { return !Ball->IsInPlay() || Ball->GetPlaneVelocity().X < 0.f; });
		const FString What = FString::Printf(TEXT("Serve at %.0f deg"), Angle);
		UTEST_TRUE(*(What + TEXT(" resolved")), bDone);
		UTEST_TRUE(*(What + TEXT(" was returned")), Ball->IsInPlay() && Ball->GetRallyHits() == 1);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPAIDriftTest, "IJPong.AI.DriftsToCentreWhenBallGoesAway", IJPAITests::Flags)
bool FIJPAIDriftTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test(IJPAITests::ArenaTransform);
	AIJPArena* Arena = Test.GetArena();
	AIJPBall* Ball = Arena->GetBall();
	AIJPPaddle* Paddle = Arena->GetPaddle(EIJPSide::Right);
	AIJPPaddleAIController* AI = IJPAITests::GetAI(Arena, EIJPSide::Right);
	UTEST_NOT_NULL("AI", AI);
	const UIJPAIProfile* Profile = IJPAITests::MakePerfectProfile();
	AI->SetProfile(Profile);

	// A 45 degree serve bounces off the top wall and arrives well above centre.
	Ball->Serve(EIJPSide::Right, 45.f);
	UTEST_TRUE("Returned", IJPAITests::RunUntil(Test, 3.f, [Ball] { return !Ball->IsInPlay() || Ball->GetPlaneVelocity().X < 0.f; }));
	const float HitY = Paddle->GetPlanePosition().Y;
	UTEST_TRUE("Made the return well off-centre", FMath::Abs(HitY) > 50.f);

	// While the ball heads away, the paddle eases back toward the middle at its idle speed.
	float FastestDrift = 0.f;
	Test.RunFor(0.4f, [&] { FastestDrift = FMath::Max(FastestDrift, FMath::Abs(Paddle->GetPlaneVelocity())); });
	UTEST_TRUE("Ball still heading away", Ball->GetPlaneVelocity().X < 0.f);
	UTEST_TRUE("Moved back toward centre", FMath::Abs(Paddle->GetPlanePosition().Y) < FMath::Abs(HitY) - 20.f);
	UTEST_TRUE("Drifts at idle speed, not full speed", FastestDrift <= Profile->IdleSpeedScale.At(AI->GetSkill()) * Paddle->GetMaxSpeed() + 1.f);
	UTEST_EQUAL_TOLERANCE("Heading for the centre", AI->GetTargetY(), 0.f, KINDA_SMALL_NUMBER);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPAIMatchTest, "IJPong.AI.DefaultProfileRalliesButConcedes", IJPAITests::Flags)
bool FIJPAIMatchTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test(IJPAITests::ArenaTransform);
	AIJPArena* Arena = Test.GetArena();
	AIJPBall* Ball = Arena->GetBall();
	AIJPTestGameMode* GameMode = Cast<AIJPTestGameMode>(Test.GetWorld()->GetAuthGameMode());
	UTEST_NOT_NULL("GameMode", GameMode);
	GameMode->RestartMatch(IJPAITests::EndlessRules());

	// The game mode's AI plays right with the project's configured profile; give it a default-profile twin on the left.
	AIJPPaddleAIController* RightAI = IJPAITests::GetAI(Arena, EIJPSide::Right);
	AIJPPaddleAIController* LeftAI = Test.GetWorld()->SpawnActor<AIJPPaddleAIController>();
	UTEST_NOT_NULL("Right AI", RightAI);
	UTEST_NOT_NULL("Left AI", LeftAI);
	LeftAI->Possess(Arena->GetPaddle(EIJPSide::Left));
	LeftAI->SetRandomSeed(1);
	RightAI->SetRandomSeed(2);

	// Two simulated minutes of AI vs AI, driven entirely by the game mode's serve loop.
	int32 TotalHits = 0;
	int32 LongestRally = 0;
	int32 LastHits = 0;
	int32 Step = 0;
	int32 LastHitStep = -1000;
	float ShortestGap = TNumericLimits<float>::Max();
	Test.RunFor(120.f, [&]
	{
		const int32 Hits = Ball->GetRallyHits();
		if (Hits > LastHits)
		{
			ShortestGap = FMath::Min(ShortestGap, (Step - LastHitStep) * FIJPTestWorld::FixedStep);
			LastHitStep = Step;
		}
		++Step;
		TotalHits += FMath::Max(0, Hits - LastHits);
		LongestRally = FMath::Max(LongestRally, Hits);
		LastHits = Hits;
	});

	const int32 Goals = GameMode->GetMatch()->GetGoals(EIJPSide::Left) + GameMode->GetMatch()->GetGoals(EIJPSide::Right);
	AddInfo(FString::Printf(TEXT("AI vs AI, 120s: %d goals (L %d - R %d), %d paddle hits, longest rally %d"),
		Goals, GameMode->GetMatch()->GetGoals(EIJPSide::Left), GameMode->GetMatch()->GetGoals(EIJPSide::Right), TotalHits, LongestRally));

	UTEST_TRUE("Beatable: goals get scored", Goals >= 3);
	UTEST_TRUE("Competent: plenty of returns", TotalHits >= 2 * Goals);
	UTEST_TRUE("Competent: real rallies happen", LongestRally >= 5);
	UTEST_TRUE("Every hit is a separate contact (>= 0.3s apart)", ShortestGap >= 0.3f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPAISkillFromArenaTest, "IJPong.AI.SkillComesFromTheArena", IJPAITests::Flags)
bool FIJPAISkillFromArenaTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test(IJPAITests::ArenaTransform);
	AIJPArena* Arena = Test.GetArena();
	AIJPPaddleAIController* AI = IJPAITests::GetAI(Arena, EIJPSide::Right);
	UTEST_NOT_NULL("AI", AI);
	UTEST_EQUAL("AI plays at the arena's opponent skill", AI->GetSkill(), Arena->GetOpponentSkill());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPAISpectrumTest, "IJPong.AI.HigherSkillBeatsLowerSkill", IJPAITests::Flags)
bool FIJPAISpectrumTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test(IJPAITests::ArenaTransform);
	AIJPArena* Arena = Test.GetArena();
	AIJPTestGameMode* GameMode = Cast<AIJPTestGameMode>(Test.GetWorld()->GetAuthGameMode());
	UTEST_NOT_NULL("GameMode", GameMode);
	GameMode->RestartMatch(IJPAITests::EndlessRules());

	// Same profile both sides; only the skill dial differs.
	AIJPPaddleAIController* Strong = IJPAITests::GetAI(Arena, EIJPSide::Right);
	AIJPPaddleAIController* Weak = Test.GetWorld()->SpawnActor<AIJPPaddleAIController>();
	UTEST_NOT_NULL("Strong AI", Strong);
	UTEST_NOT_NULL("Weak AI", Weak);
	Weak->Possess(Arena->GetPaddle(EIJPSide::Left));
	Strong->SetSkill(0.85f);
	Weak->SetSkill(0.15f);
	Strong->SetRandomSeed(3);
	Weak->SetRandomSeed(4);

	Test.RunFor(120.f);

	const int32 StrongScore = GameMode->GetMatch()->GetGoals(EIJPSide::Right);
	const int32 WeakScore = GameMode->GetMatch()->GetGoals(EIJPSide::Left);
	AddInfo(FString::Printf(TEXT("Skill 0.85 vs 0.15 over 120s: %d - %d"), StrongScore, WeakScore));
	UTEST_TRUE("Stronger AI outscores the weaker one clearly", StrongScore >= WeakScore + 3);
	return true;
}

#endif
