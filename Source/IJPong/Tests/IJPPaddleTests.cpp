// It's Just Pong

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "CollisionShape.h"
#include "Core/IJPTestGameMode.h"
#include "Core/IJPTypes.h"
#include "Engine/World.h"
#include "Gameplay/IJPArena.h"
#include "Gameplay/IJPPaddle.h"
#include "Gameplay/IJPPaddleClass.h"
#include "Gameplay/IJPPaddleProfile.h"
#include "GameFramework/Controller.h"
#include "Tests/IJPTestWorld.h"

namespace IJPPaddleTests
{
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter;

	// Rotated and offset so every check also exercises the plane <-> world mapping.
	const FTransform ArenaTransform(FRotator(0.f, 30.f, 0.f), FVector(500.f, -200.f, 300.f));
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPPaddleSpawnTest, "IJPong.Paddle.SpawnsInLanes", IJPPaddleTests::Flags)
bool FIJPPaddleSpawnTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test(IJPPaddleTests::ArenaTransform);
	AIJPArena* Arena = Test.GetArena();

	UTEST_NOT_NULL("Arena", Arena);
	UTEST_TRUE("Project default game mode is AIJPTestGameMode", Test.GetWorld()->GetAuthGameMode()->IsA<AIJPTestGameMode>());
	UTEST_EQUAL("Game mode found the arena", Cast<AIJPTestGameMode>(Test.GetWorld()->GetAuthGameMode())->GetArena(), Arena);

	for (EIJPSide Side : { EIJPSide::Left, EIJPSide::Right })
	{
		const AIJPPaddle* Paddle = Arena->GetPaddle(Side);
		UTEST_NOT_NULL("Paddle spawned", Paddle);
		UTEST_EQUAL("Paddle side", Paddle->GetSide(), Side);
		UTEST_EQUAL("Paddle arena", Paddle->GetArena(), Arena);

		const FVector2D Expected(Arena->GetLaneX(Side), 0.f);
		UTEST_EQUAL_TOLERANCE("Starts in its lane", Paddle->GetPlanePosition().X, Expected.X, 1e-4);
		UTEST_EQUAL_TOLERANCE("Starts vertically centred", Paddle->GetPlanePosition().Y, Expected.Y, 1e-4);
		UTEST_EQUAL_TOLERANCE("World position matches plane position", Paddle->GetActorLocation(), Arena->PlaneToWorld(Expected), 0.01f);
		UTEST_TRUE("Lane is on its own side of the net", IJP::SideSign(Side) * Expected.X > 0.f);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPPaddleRampTest, "IJPong.Paddle.RampsToMaxSpeedAndStops", IJPPaddleTests::Flags)
bool FIJPPaddleRampTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test(IJPPaddleTests::ArenaTransform);
	AIJPPaddle* Paddle = Test.GetArena()->GetPaddle(EIJPSide::Left);
	UTEST_NOT_NULL("Paddle", Paddle);
	const float MaxSpeed = Paddle->GetMaxSpeed();
	const float RampTime = Paddle->GetRampTime();
	const float Step = FIJPTestWorld::FixedStep;
	auto HoldUp = [Paddle] { Paddle->AddMoveInput(1.f); };

	// One frame in: moving up, but still ramping.
	Test.RunFor(Step, HoldUp);
	UTEST_TRUE("Moving up after one frame", Paddle->GetPlaneVelocity() > 0.f);
	UTEST_TRUE("Not at full speed after one frame", Paddle->GetPlaneVelocity() < MaxSpeed);
	UTEST_TRUE("Moved up", Paddle->GetPlanePosition().Y > 0.f);

	// By the end of the ramp (plus a frame for rounding): exactly full speed.
	Test.RunFor(RampTime, HoldUp);
	UTEST_EQUAL_TOLERANCE("Full speed after RampTime", Paddle->GetPlaneVelocity(), MaxSpeed, KINDA_SMALL_NUMBER);

	// Input is consumed per frame: with nothing held the paddle ramps back to rest and stays put.
	Test.RunFor(RampTime + Step);
	UTEST_EQUAL_TOLERANCE("Stopped after release", Paddle->GetPlaneVelocity(), 0.f, KINDA_SMALL_NUMBER);
	const double RestY = Paddle->GetPlanePosition().Y;
	Test.RunFor(0.25f);
	UTEST_EQUAL_TOLERANCE("Stays put with no input", Paddle->GetPlanePosition().Y, RestY, 1e-4);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPPaddleClampTest, "IJPong.Paddle.StopsAtWalls", IJPPaddleTests::Flags)
bool FIJPPaddleClampTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test(IJPPaddleTests::ArenaTransform);
	AIJPArena* Arena = Test.GetArena();
	AIJPPaddle* Paddle = Arena->GetPaddle(EIJPSide::Right);
	UTEST_NOT_NULL("Paddle", Paddle);
	const float Limit = Arena->GetHalfExtents().Y - Paddle->GetSize().Y * 0.5f;

	// Only this test steers the paddle: its AI would add its own input (full speed toward a ball
	// served its way) and could cancel ours out.
	if (AController* AI = Paddle->GetController())
	{
		AI->UnPossess();
	}

	// Hold well past the time needed to cross the whole field, in both directions.
	for (const float Dir : { 1.f, -1.f })
	{
		Test.RunFor(2.f, [Paddle, Dir] { Paddle->AddMoveInput(Dir); });
		UTEST_EQUAL_TOLERANCE("Edge flush with the wall", Paddle->GetPlanePosition().Y, double(Dir * Limit), 1e-4);
		UTEST_EQUAL_TOLERANCE("No velocity while pinned to the wall", Paddle->GetPlaneVelocity(), 0.f, KINDA_SMALL_NUMBER);
		UTEST_EQUAL_TOLERANCE("Stays in its lane", Paddle->GetPlanePosition().X, double(Arena->GetLaneX(EIJPSide::Right)), 1e-4);
	}

	// Input over +-1 (e.g. keyboard and stick at once) is clamped, not added. From the bottom wall,
	// 0.2s at full speed is still mid-field, so the paddle is moving freely when we check.
	Test.RunFor(0.2f, [Paddle] { Paddle->AddMoveInput(1.f); Paddle->AddMoveInput(1.f); });
	UTEST_EQUAL_TOLERANCE("Doubled input still caps at MaxSpeed", Paddle->GetPlaneVelocity(), Paddle->GetMaxSpeed(), KINDA_SMALL_NUMBER);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPPaddleBlocksBallTest, "IJPong.Paddle.BlocksBallSweep", IJPPaddleTests::Flags)
bool FIJPPaddleBlocksBallTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test(IJPPaddleTests::ArenaTransform);
	AIJPArena* Arena = Test.GetArena();
	AIJPPaddle* Paddle = Arena->GetPaddle(EIJPSide::Left);
	UTEST_NOT_NULL("Paddle", Paddle);

	// Move the paddle off-centre first, so the sweep only hits it if its collision moved with it.
	Test.RunFor(0.3f, [Paddle] { Paddle->AddMoveInput(1.f); });
	const FVector2D PaddlePos = Paddle->GetPlanePosition();
	UTEST_TRUE("Paddle moved off centre", PaddlePos.Y > 50.f);

	// A ball-sized sweep from mid-field straight at the paddle, along the ball's channel.
	const FCollisionShape Ball = FCollisionShape::MakeSphere(5.f);
	auto Sweep = [&](float PlaneY, FHitResult& OutHit)
	{
		const FVector Start = Arena->PlaneToWorld(FVector2D(0.f, PlaneY));
		const FVector End = Arena->PlaneToWorld(FVector2D(-Arena->GetHalfExtents().X, PlaneY));
		return Test.GetWorld()->SweepSingleByChannel(OutHit, Start, End, FQuat::Identity, ECC_PongBall, Ball);
	};

	FHitResult Hit;
	UTEST_TRUE("Sweep at the paddle hits something", Sweep(PaddlePos.Y, Hit));
	UTEST_EQUAL("Sweep at the paddle hits the paddle", Hit.GetActor(), static_cast<AActor*>(Paddle));

	// Where the paddle used to be, the ball passes through to the goal line.
	FHitResult Miss;
	const bool bMissHit = Sweep(0.f, Miss);
	UTEST_TRUE("Sweep where the paddle was doesn't hit the paddle", !bMissHit || Miss.GetActor() != Paddle);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPPaddleProfileTest, "IJPong.Paddle.EachSideUsesItsProfile", IJPPaddleTests::Flags)
bool FIJPPaddleProfileTest::RunTest(const FString& Parameters)
{
	// A long, slow paddle for the left side only. (Left, because in tests it has no controller.)
	UIJPPaddleProfile* Long = NewObject<UIJPPaddleProfile>();
	Long->Size = FVector2D(12.f, 140.f);
	Long->MaxSpeed = 300.f;

	UIJPPaddleClass* LongClass = NewObject<UIJPPaddleClass>();
	LongClass->Profile = Long;
	FIJPTestWorld Test(IJPPaddleTests::ArenaTransform, [LongClass](AIJPArena& Arena) { Arena.SetPaddleClass(EIJPSide::Left, LongClass); });
	AIJPArena* Arena = Test.GetArena();
	AIJPPaddle* Left = Arena->GetPaddle(EIJPSide::Left);
	AIJPPaddle* Right = Arena->GetPaddle(EIJPSide::Right);

	UTEST_EQUAL("Left uses the long profile", Left->GetSize().Y, 140.0);
	UTEST_TRUE("Right keeps its own profile", Right->GetSize().Y != 140.0);

	// The profile drives the real movement and wall limits, not just the numbers.
	Test.RunFor(0.3f, [Left] { Left->AddMoveInput(1.f); });
	UTEST_EQUAL_TOLERANCE("Capped at the profile's speed", Left->GetPlaneVelocity(), 300.f, 1e-3f);
	Test.RunFor(2.f, [Left] { Left->AddMoveInput(1.f); });
	UTEST_EQUAL_TOLERANCE("Long paddle's edge sits on the wall", Left->GetPlanePosition().Y, double(Arena->GetHalfExtents().Y - 70.f), 1e-3);
	return true;
}

#endif
