// It's Just Pong

#include "Gameplay/IJPPaddle.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Gameplay/IJPArena.h"
#include "UObject/ConstructorHelpers.h"

AIJPPaddle::AIJPPaddle()
{
	PrimaryActorTick.bCanEverTick = true;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));

	Collision = CreateDefaultSubobject<UBoxComponent>(TEXT("Collision"));
	SetRootComponent(Collision);
	IJP::ConfigureAsBallBlocker(Collision);

	Visual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Visual"));
	Visual->SetupAttachment(Collision);
	Visual->SetStaticMesh(CubeMesh.Object);
	Visual->CastShadow = false;
	IJP::ConfigureAsVisualOnly(Visual);

	// The arena camera is the view; the paddle never owns one.
	bFindCameraComponentWhenViewTarget = false;
}

void AIJPPaddle::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// Same local axes as the arena: X = plane X, Z = plane Y, Y = depth.
	Collision->SetBoxExtent(FVector(Size.X * 0.5f, BlockerDepth * 0.5f, Size.Y * 0.5f));

	const float CubeSize = 100.f; // /Engine/BasicShapes/Cube is 100 units, centred.
	Visual->SetRelativeScale3D(FVector(Size.X / CubeSize, VisualDepth / CubeSize, Size.Y / CubeSize));
}

void AIJPPaddle::InitPaddle(AIJPArena* InArena, EIJPSide InSide, float InLaneX)
{
	check(InArena);
	Arena = InArena;
	Side = InSide;
	LaneX = InLaneX;
	PlaneY = 0.f;
	Velocity = 0.f;
	PendingInput = 0.f;

	Visual->SetMaterial(0, InArena->GetPongMaterial());
	UpdateTransform();
}

void AIJPPaddle::AddMoveInput(float Value)
{
	PendingInput += Value;
}

void AIJPPaddle::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Consume this frame's input, like APawn's movement input vector.
	const float Input = FMath::Clamp(PendingInput, -1.f, 1.f);
	PendingInput = 0.f;

	AIJPArena* ArenaPtr = Arena.Get();
	if (!ArenaPtr)
	{
		return;
	}

	// Ramp toward the target speed at a constant rate, so reaching MaxSpeed from rest takes RampTime.
	const float TargetVelocity = Input * MaxSpeed;
	if (RampTime <= 0.f)
	{
		Velocity = TargetVelocity;
	}
	else
	{
		Velocity = FMath::FInterpConstantTo(Velocity, TargetVelocity, DeltaSeconds, MaxSpeed / RampTime);
	}

	// Stay between the walls. Hitting a wall kills the velocity, so the paddle doesn't "push" into it.
	const float MaxY = FMath::Max(0.f, ArenaPtr->GetHalfExtents().Y - Size.Y * 0.5f);
	const float NewY = PlaneY + Velocity * DeltaSeconds;
	PlaneY = FMath::Clamp(NewY, -MaxY, MaxY);
	if (PlaneY != NewY)
	{
		Velocity = 0.f;
	}

	UpdateTransform();
}

void AIJPPaddle::UpdateTransform()
{
	const AIJPArena* ArenaPtr = Arena.Get();
	check(ArenaPtr);
	SetActorLocationAndRotation(ArenaPtr->PlaneToWorld(GetPlanePosition()), ArenaPtr->GetActorQuat());
}
