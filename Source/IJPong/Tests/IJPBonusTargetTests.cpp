// It's Just Pong

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Core/IJPRunGameMode.h"
#include "Core/IJPTypes.h"
#include "Engine/World.h"
#include "Era/IJPEra.h"
#include "Era/IJPEraSubsystem.h"
#include "Gameplay/IJPArena.h"
#include "Gameplay/IJPBall.h"
#include "Gameplay/IJPBonusTarget.h"
#include "Gameplay/IJPBonusTargetComponent.h"
#include "Gameplay/IJPDriftingBlockComponent.h"
#include "Gameplay/IJPLightTrailComponent.h"
#include "Gameplay/IJPBulletTimeComponent.h"
#include "Camera/CameraComponent.h"
#include "Gameplay/IJPMatchComponent.h"
#include "Gameplay/IJPMatchRules.h"
#include "Gameplay/IJPPaddle.h"
#include "Run/IJPActConfig.h"
#include "Run/IJPRunSubsystem.h"
#include "Tests/IJPTestWorld.h"

namespace IJPBonusTargetTests
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

	UIJPEra* MakeEra(float Interval)
	{
		UIJPEra* Era = NewObject<UIJPEra>(GetTransientPackage());
		Era->BonusTargets.bEnabled = true;
		Era->BonusTargets.SpawnInterval = Interval;
		Era->BonusTargets.MaxTargets = 2;
		Era->BonusTargets.Coins = 5;
		return Era;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPBonusTargetSpawnTest, "IJPong.Era.BonusTargetsAppearWhileABallIsInPlay", IJPBonusTargetTests::Flags)
bool FIJPBonusTargetSpawnTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test;
	AIJPArena* Arena = Test.GetArena();
	UIJPBonusTargetComponent* Targets = Arena->GetBonusTargets();
	UTEST_EQUAL("None in an era without them", Targets->GetNumTargets(), 0);

	UIJPEraSubsystem::Get(Arena)->SetEra(IJPBonusTargetTests::MakeEra(0.5f));
	Test.RunFor(2.f);
	UTEST_TRUE("Some appear", Targets->GetNumTargets() >= 1);
	Test.RunFor(3.f);
	UTEST_TRUE("Never more than the most", Targets->GetNumTargets() <= 2);

	UIJPEraSubsystem::Get(Arena)->SetEra(NewObject<UIJPEra>(GetTransientPackage()));
	UTEST_EQUAL("Gone with the era", Targets->GetNumTargets(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPBumperTest, "IJPong.Boss.BumpersKickTheBallAndStay", IJPBonusTargetTests::Flags)
bool FIJPBumperTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test;
	AIJPArena* Arena = Test.GetArena();
	AIJPBall* Ball = Arena->GetBall();
	Test.RunFor(1.1f);

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AIJPBonusTarget* Bumper = Test.GetWorld()->SpawnActor<AIJPBonusTarget>(AIJPBonusTarget::StaticClass(), Arena->GetActorTransform(), Params);
	Bumper->InitBumper(Arena, FVector2D(100.f, 0.f), FVector2D(24.f), 1.5f, EIJPPaletteRole::RightPaddle);
	Ball->Launch(FVector2D(0.f, 0.f), FVector2D(300.f, 0.f));
	bool bBounced = false;
	for (int32 i = 0; i < 60 && !bBounced; ++i)
	{
		Test.Step();
		bBounced = Ball->GetPlaneVelocity().X < 0.f;
	}
	UTEST_TRUE("Bounced off", bBounced);
	UTEST_TRUE("Kicked", Ball->GetPlaneVelocity().Size() > 400.f);
	UTEST_TRUE("Still there", IsValid(Bumper) && !Bumper->IsHidden());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPDriftingBlocksTest, "IJPong.Era.DriftingBlocksMoveStayInsideAndDeflect", IJPBonusTargetTests::Flags)
bool FIJPDriftingBlocksTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test;
	AIJPArena* Arena = Test.GetArena();
	AIJPBall* Ball = Arena->GetBall();
	UIJPDriftingBlockComponent* Drift = Arena->GetDriftingBlocks();
	UTEST_EQUAL("None in an era without them", Drift->GetNumBlocks(), 0);

	UIJPEra* Era = NewObject<UIJPEra>(GetTransientPackage());
	Era->DriftingBlocks.bEnabled = true;
	Era->DriftingBlocks.Count = 2;
	Era->DriftingBlocks.Speed = 200.f;
	UIJPEraSubsystem::Get(Arena)->SetEra(Era);
	Test.Step();
	UTEST_EQUAL("Two blocks", Drift->GetNumBlocks(), 2);
	AIJPBonusTarget* Block = Drift->GetBlocks()[0];
	const double StartY = Block->GetPlanePosition().Y;
	Test.RunFor(0.2f);
	UTEST_TRUE("Drifting", FMath::Abs(Block->GetPlanePosition().Y - StartY) > 10.0);

	// Long enough to reach a wall and turn back: always inside.
	const float Limit = Arena->GetHalfExtents().Y - Era->DriftingBlocks.Size.Y * 0.5f + 0.01f;
	bool bInside = true;
	Test.RunFor(5.f, [&] { bInside &= FMath::Abs(Block->GetPlanePosition().Y) <= Limit; });
	UTEST_TRUE("Never through the walls", bInside);

	// A ball sent straight at it bounces back.
	Era->DriftingBlocks.Speed = 0.f;
	const FVector2D At = Block->GetPlanePosition();
	const float Dir = At.X < 0.f ? -1.f : 1.f;
	Ball->Launch(FVector2D(At.X - Dir * 120.f, At.Y), FVector2D(Dir * 300.f, 0.f));
	bool bBounced = false;
	for (int32 i = 0; i < 60 && !bBounced; ++i)
	{
		Test.Step();
		bBounced = Ball->GetPlaneVelocity().X * Dir < 0.f;
	}
	UTEST_TRUE("Deflected", bBounced);
	UTEST_FALSE("Not kicked", Ball->IsBoosted());

	UIJPEraSubsystem::Get(Arena)->SetEra(NewObject<UIJPEra>(GetTransientPackage()));
	UTEST_EQUAL("Gone with the era", Drift->GetNumBlocks(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPLightTrailsTest, "IJPong.Era.LightTrailsBlockCrossingBallsButNotAlongThem", IJPBonusTargetTests::Flags)
bool FIJPLightTrailsTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test;
	AIJPArena* Arena = Test.GetArena();
	AIJPBall* Ball = Arena->GetBall();
	UIJPLightTrailComponent* Trails = Arena->GetLightTrails();
	Test.RunFor(1.1f);

	UIJPEra* Era = NewObject<UIJPEra>(GetTransientPackage());
	Era->LightTrails.bEnabled = true;
	Era->BloomIntensity = 2.f;
	UIJPEraSubsystem::Get(Arena)->SetEra(Era);
	UTEST_EQUAL_TOLERANCE("The camera glows", Arena->GetCamera()->PostProcessSettings.BloomIntensity, 2.f, 0.001f);

	// A ball going up lays a vertical trail.
	Ball->Launch(FVector2D(60.f, -200.f), FVector2D(0.f, 400.f));
	Test.RunFor(0.5f);
	UTEST_TRUE("A trail", Trails->GetNumPieces() > 3);
	UTEST_TRUE("Solid behind the ball", Trails->GetNumSolidPieces() > 0);

	// Another ball crossing it bounces back.
	AIJPBall* Crosser = Arena->AddBall(nullptr);
	Crosser->Launch(FVector2D(-40.f, -100.f), FVector2D(400.f, 0.f));
	bool bBounced = false;
	for (int32 i = 0; i < 40 && !bBounced; ++i)
	{
		Test.Step();
		bBounced = Crosser->GetPlaneVelocity().X < 0.f;
	}
	UTEST_TRUE("Bounced off the trail", bBounced);
	UTEST_TRUE("Before passing it", Crosser->GetPlanePosition().X < 60.f);
	Crosser->ResetBall();

	// A flat return heads back along its own wake without bouncing off it.
	Test.RunFor(1.2f); // the old trail fades
	AIJPPaddle* Left = Arena->GetPaddle(EIJPSide::Left);
	Ball->Launch(FVector2D(0.f, Left->GetPlanePosition().Y), FVector2D(-400.f, 0.f));
	UTEST_TRUE("Returned", IJPBonusTargetTests::RunUntil(Test, 2.f, [Ball] { return Ball->GetRallyHits() >= 1; }));
	Test.RunFor(0.5f);
	UTEST_TRUE("Still heading out along the wake", Ball->GetPlaneVelocity().X > 0.f && Ball->GetPlanePosition().X > Left->GetPlanePosition().X + 120.f);
	UTEST_EQUAL("Returned once, not ping-ponged", Ball->GetRallyHits(), 1);

	UIJPEraSubsystem::Get(Arena)->SetEra(NewObject<UIJPEra>(GetTransientPackage()));
	UTEST_EQUAL("Gone with the era", Trails->GetNumPieces(), 0);
	UTEST_EQUAL_TOLERANCE("No glow", Arena->GetCamera()->PostProcessSettings.BloomIntensity, 0.f, 0.001f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPBulletTimeTest, "IJPong.Era.BulletTimeSlowsANearGoalOnce", IJPBonusTargetTests::Flags)
bool FIJPBulletTimeTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test;
	AIJPArena* Arena = Test.GetArena();
	AIJPBall* Ball = Arena->GetBall();
	AIJPPaddle* Left = Arena->GetPaddle(EIJPSide::Left);
	const auto HoldUp = [Left] { Left->AddMoveInput(1.f); };
	UIJPBulletTimeComponent* Bullet = Arena->GetBulletTime();
	UIJPEra* Era = NewObject<UIJPEra>(GetTransientPackage());
	Era->BulletTime.bEnabled = true;
	Era->BulletTime.TimeScale = 0.25f;
	Era->BulletTime.Duration = 0.5f;
	UIJPEraSubsystem::Get(Arena)->SetEra(Era);
	Test.RunFor(1.1f, HoldUp);

	// The paddle's up at the top; a ball straight at the middle of its goal.
	Ball->Serve(EIJPSide::Left, 0.f);
	UTEST_TRUE("Bullet time", IJPBonusTargetTests::RunUntil(Test, 2.f, [Bullet] { return Bullet->IsActive(); }));
	UTEST_EQUAL_TOLERANCE("Balls at a quarter speed", Arena->GetBallTimeFactor(), 0.25f, 0.001f);
	const double X0 = Ball->GetPlanePosition().X;
	Test.RunFor(0.2f, HoldUp);
	const double Moved = FMath::Abs(Ball->GetPlanePosition().X - X0);
	UTEST_TRUE("Crawling", Moved < Ball->GetPlaneVelocity().Size() * 0.2 * 0.4);

	// Once per approach: it ends and doesn't come back for the same ball.
	bool bAgain = false;
	Test.RunFor(0.4f, HoldUp);
	UTEST_FALSE("Over", Bullet->IsActive());
	Test.RunFor(0.5f, [&] { HoldUp(); bAgain |= Bullet->IsActive(); });
	UTEST_FALSE("Not twice", bAgain);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPBonusTargetCoinsTest, "IJPong.Run.BreakingABonusTargetPaysCoinsAndBounces", IJPBonusTargetTests::Flags)
bool FIJPBonusTargetCoinsTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test(FTransform::Identity, nullptr, AIJPRunGameMode::StaticClass());
	AIJPRunGameMode* Mode = Cast<AIJPRunGameMode>(Test.GetWorld()->GetAuthGameMode());
	UIJPRunSubsystem* Run = UIJPRunSubsystem::Get(Test.GetWorld());
	UIJPActConfig* Act = NewObject<UIJPActConfig>(GetTransientPackage());
	Act->Rows = 3;
	Act->Lanes = 1;
	Act->Paths = 1;
	Mode->StartNewRun(Act, 1, 5);
	UIJPEraSubsystem::Get(Test.GetWorld())->SetEra(IJPBonusTargetTests::MakeEra(100.f)); // only the one placed here
	Mode->HandleUIConfirm();
	UTEST_EQUAL("Playing", Mode->GetPhase(), EIJPRunPhase::Playing);
	Test.RunFor(1.2f);

	// A target in the player's line: a flat return breaks it and comes back.
	AIJPArena* Arena = Mode->GetArena();
	AIJPPaddle* Player = Arena->GetPaddle(EIJPSide::Left);
	AIJPBall* Ball = Arena->GetBall();
	const FVector2D Face = Player->GetPlanePosition() + FVector2D(Player->GetSize().X * 0.5f, 0.f);
	Arena->GetBonusTargets()->SpawnTargetAt(FVector2D(0.f, Player->GetPlanePosition().Y));
	Ball->Launch(Face + FVector2D(60.f, 0.f), FVector2D(-400.f, 0.f));
	const int32 CoinsBefore = Run->GetCoins();
	bool bBroken = false;
	for (int32 i = 0; i < 180 && !bBroken; ++i)
	{
		Test.Step();
		bBroken = Arena->GetBonusTargets()->GetNumTargets() == 0;
	}
	UTEST_TRUE("Broken", bBroken);
	UTEST_EQUAL("Paid", Run->GetCoins(), CoinsBefore + 5);
	UTEST_TRUE("Bounced back", Ball->GetPlaneVelocity().X < 0.f);
	return true;
}

#endif
