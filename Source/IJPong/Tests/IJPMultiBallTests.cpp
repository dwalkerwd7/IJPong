// It's Just Pong

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "AI/IJPPaddleAIController.h"
#include "Core/IJPTestGameMode.h"
#include "Core/IJPTypes.h"
#include "Engine/World.h"
#include "Era/IJPEra.h"
#include "Era/IJPEraSubsystem.h"
#include "Gameplay/IJPArena.h"
#include "Gameplay/IJPBall.h"
#include "Gameplay/IJPBallType.h"
#include "Gameplay/IJPMatchComponent.h"
#include "Gameplay/IJPMatchRules.h"
#include "Gameplay/IJPPaddle.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "Tests/IJPTestWorld.h"

namespace IJPMultiBallTests
{
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter;

	AIJPTestGameMode* GetMode(const FIJPTestWorld& Test)
	{
		return Cast<AIJPTestGameMode>(Test.GetWorld()->GetAuthGameMode());
	}

	/** Step until Condition holds or MaxSeconds pass, calling BeforeEachStep first on every step. */
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

	UIJPBallType* MakeType(float Size, float BaseSpeed, int32 Points, const FLinearColor& Colour = FLinearColor::White)
	{
		UIJPBallType* Type = NewObject<UIJPBallType>(GetTransientPackage());
		Type->Size = Size;
		Type->BaseSpeed = BaseSpeed;
		Type->Points = Points;
		Type->Colour = Colour;
		return Type;
	}

	FLinearColor ColorOf(const AActor* Actor)
	{
		FLinearColor Color = FLinearColor::Transparent;
		const UStaticMeshComponent* Mesh = Actor->FindComponentByClass<UStaticMeshComponent>();
		if (const UMaterialInterface* Material = Mesh ? Mesh->GetMaterial(0) : nullptr)
		{
			Material->GetVectorParameterValue(FHashedMaterialParameterInfo(TEXT("Color")), Color);
		}
		return Color;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPBallTypeTest, "IJPong.Ball.TypeSetsSizeSpeedAndPoints", IJPMultiBallTests::Flags)
bool FIJPBallTypeTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test;
	AIJPTestGameMode* Mode = IJPMultiBallTests::GetMode(Test);
	AIJPArena* Arena = Mode->GetArena();
	AIJPBall* Ball = Arena->GetBall();
	AIJPPaddle* Left = Arena->GetPaddle(EIJPSide::Left);
	Test.RunFor(1.1f); // past the match's first serve, so our own serve below is the only one

	UIJPBallType* Heavy = IJPMultiBallTests::MakeType(16.f, 250.f, 2);
	Ball->SetType(Heavy);
	UTEST_EQUAL_TOLERANCE("Size from the type", Ball->GetSize(), 16.f, KINDA_SMALL_NUMBER);

	// Left paddle held out of the way; straight at its goal.
	const int32 RightBefore = Mode->GetMatch()->GetScore(EIJPSide::Right);
	Ball->Serve(EIJPSide::Left, 0.f);
	UTEST_EQUAL_TOLERANCE("Served at the type's speed", Ball->GetPlaneVelocity().Size(), 250.0, 0.01);
	const bool bScored = IJPMultiBallTests::RunUntil(Test, 4.f, [Ball] { return !Ball->IsInPlay(); }, [Left] { Left->AddMoveInput(1.f); });
	UTEST_TRUE("Goal", bScored);
	UTEST_EQUAL("Worth the type's points", Mode->GetMatch()->GetScore(EIJPSide::Right), RightBefore + 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPEachBallScoresTest, "IJPong.Match.EachBallScoresAloneThenServe", IJPMultiBallTests::Flags)
bool FIJPEachBallScoresTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test;
	AIJPTestGameMode* Mode = IJPMultiBallTests::GetMode(Test);
	AIJPArena* Arena = Mode->GetArena();
	UIJPMatchComponent* Match = Mode->GetMatch();
	AIJPBall* Main = Arena->GetBall();
	AIJPPaddle* Left = Arena->GetPaddle(EIJPSide::Left);
	const auto HoldUp = [Left] { Left->AddMoveInput(1.f); };
	Test.RunFor(1.1f);

	// Two balls at the left goal (paddle held at the top): one straight, one later and lower.
	const int32 RightBefore = Match->GetScore(EIJPSide::Right);
	Main->Serve(EIJPSide::Left, 0.f);
	AIJPBall* Extra = Arena->AddBall(nullptr);
	UTEST_TRUE("A second ball", Extra && Extra != Main);
	Test.RunFor(0.4f, HoldUp);
	Extra->Serve(EIJPSide::Left, -20.f);
	UTEST_EQUAL("Two in play", Arena->GetNumBallsInPlay(), 2);

	UTEST_TRUE("First goal", IJPMultiBallTests::RunUntil(Test, 3.f, [Main] { return !Main->IsInPlay(); }, HoldUp));
	UTEST_EQUAL("Scored alone", Match->GetScore(EIJPSide::Right), RightBefore + 1);
	UTEST_TRUE("The other ball plays on", Extra->IsInPlay());
	UTEST_EQUAL("Still a rally", Match->GetPhase(), EIJPMatchPhase::Rally);
	UTEST_FALSE("No serve while a ball is live", Main->IsBlinking());

	UTEST_TRUE("Second goal", IJPMultiBallTests::RunUntil(Test, 3.f, [Extra] { return !Extra->IsInPlay(); }, HoldUp));
	UTEST_EQUAL("Scored too", Match->GetScore(EIJPSide::Right), RightBefore + 2);
	UTEST_EQUAL("Court empty: serve", Match->GetPhase(), EIJPMatchPhase::Serve);
	UTEST_TRUE("Main ball blinks for it", Main->IsBlinking());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPServeListTest, "IJPong.Match.ServeListLaunchesEveryType", IJPMultiBallTests::Flags)
bool FIJPServeListTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test;
	AIJPTestGameMode* Mode = IJPMultiBallTests::GetMode(Test);
	AIJPArena* Arena = Mode->GetArena();

	UIJPBallType* First = IJPMultiBallTests::MakeType(12.f, 300.f, 1);
	UIJPBallType* Second = IJPMultiBallTests::MakeType(8.f, 500.f, 1);
	UIJPMatchRules* Rules = NewObject<UIJPMatchRules>(GetTransientPackage());
	Rules->ServeDelay = 0.5f;
	Rules->ServedBalls = { First, Second };

	Mode->RestartMatch(Rules);
	UTEST_TRUE("Main ball blinks as the first type", Arena->GetBall()->IsBlinking() && &Arena->GetBall()->GetType() == First);

	Test.RunFor(0.6f);
	UTEST_EQUAL("Both served", Arena->GetNumBallsInPlay(), 2);
	const AIJPBall* SecondBall = nullptr;
	for (const AIJPBall* Ball : Arena->GetBalls())
	{
		if (Ball->IsInPlay() && &Ball->GetType() == Second)
		{
			SecondBall = Ball;
		}
	}
	UTEST_NOT_NULL("Second type in play", SecondBall);
	UTEST_TRUE("Served in opposite directions", Arena->GetBall()->GetPlaneVelocity().X * SecondBall->GetPlaneVelocity().X < 0.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPBallTypeColourTest, "IJPong.Era.BallTypeColoursOnlyWhenTheEraShowsThem", IJPMultiBallTests::Flags)
bool FIJPBallTypeColourTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test;
	AIJPArena* Arena = Test.GetArena();
	UIJPEraSubsystem* Eras = UIJPEraSubsystem::Get(Arena);

	AIJPBall* Red = Arena->AddBall(IJPMultiBallTests::MakeType(10.f, 400.f, 1, FLinearColor::Red));
	UTEST_EQUAL("Black-and-white era: palette colour", IJPMultiBallTests::ColorOf(Red), Eras->GetEra()->Palette.Ball);

	UIJPEra* Colourful = NewObject<UIJPEra>(GetTransientPackage());
	Colourful->Palette.Ball = FLinearColor::Green;
	Colourful->Palette.bBallTypeColours = true;
	Eras->SetEra(Colourful);
	UTEST_EQUAL("Colour era: the type's colour", IJPMultiBallTests::ColorOf(Red), FLinearColor::Red);
	UTEST_EQUAL("Main ball: its own type's colour", IJPMultiBallTests::ColorOf(Arena->GetBall()), Arena->GetBall()->GetType().Colour);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPAIPicksBallTest, "IJPong.AI.DefendsTheBallThatArrivesFirst", IJPMultiBallTests::Flags)
bool FIJPAIPicksBallTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test;
	AIJPArena* Arena = Test.GetArena();
	Test.RunFor(1.1f);

	AIJPPaddleAIController* AI = Cast<AIJPPaddleAIController>(Arena->GetPaddle(EIJPSide::Right)->GetController());
	UTEST_NOT_NULL("Right paddle has an AI", AI);
	AI->SetSkill(1.f);

	// The main ball heads away from the AI; a second one comes at it, rising.
	Arena->GetBall()->Serve(EIJPSide::Left, 0.f);
	AIJPBall* Incoming = Arena->AddBall(nullptr);
	Incoming->Serve(EIJPSide::Right, 25.f);
	Test.RunFor(0.5f);
	UTEST_TRUE("Heads for the incoming ball's intercept, not the middle", AI->GetTargetY() > 60.f);
	return true;
}

#endif
