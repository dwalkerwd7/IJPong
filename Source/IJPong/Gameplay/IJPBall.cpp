// It's Just Pong

#include "Gameplay/IJPBall.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Gameplay/IJPArena.h"
#include "Gameplay/IJPGoalComponent.h"
#include "Gameplay/IJPPaddle.h"
#include "Gameplay/IJPPongMath.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	// Gap left between the ball and whatever it touched, so the next sweep starts clear of it.
	constexpr float ContactSkin = 0.1f;
}

AIJPBall::AIJPBall()
{
	PrimaryActorTick.bCanEverTick = true;
	// Paddles move in their own tick; sweeping after them means the ball sees where they are this frame.
	PrimaryActorTick.TickGroup = TG_PostPhysics;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));

	// The ball has no collision of its own: it only ever sweeps a shape through the world.
	Visual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Visual"));
	SetRootComponent(Visual);
	Visual->SetStaticMesh(CubeMesh.Object);
	Visual->CastShadow = false;
	IJP::ConfigureAsVisualOnly(Visual);
}

void AIJPBall::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	const float CubeSize = 100.f; // /Engine/BasicShapes/Cube is 100 units, centred.
	Visual->SetRelativeScale3D(FVector(Size / CubeSize, VisualDepth / CubeSize, Size / CubeSize));
}

void AIJPBall::InitBall(AIJPArena* InArena)
{
	check(InArena);
	Arena = InArena;
	Visual->SetMaterial(0, InArena->GetPongMaterial());
	ResetBall();
}

void AIJPBall::Serve(EIJPSide Toward, float AngleDeg)
{
	const float AngleRad = FMath::DegreesToRadians(FMath::Clamp(AngleDeg, -MaxBounceAngleDeg, MaxBounceAngleDeg));

	Position = PreviousPosition = FVector2D::ZeroVector;
	Speed = BaseSpeed;
	Velocity = FVector2D(IJP::SideSign(Toward) * FMath::Cos(AngleRad), FMath::Sin(AngleRad)) * Speed;
	Accumulator = 0.f;
	RallyHits = 0;
	bInPlay = true;

	SetActorHiddenInGame(false);
	UpdateDrawnTransform(1.f);
}

void AIJPBall::ResetBall()
{
	Position = PreviousPosition = FVector2D::ZeroVector;
	Velocity = FVector2D::ZeroVector;
	Speed = 0.f;
	Accumulator = 0.f;
	RallyHits = 0;
	bInPlay = false;

	SetActorHiddenInGame(true);
	if (Arena.IsValid())
	{
		UpdateDrawnTransform(1.f);
	}
}

void AIJPBall::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bInPlay || !Arena.IsValid())
	{
		return;
	}

	// Fixed substeps: same result at any frame rate. The cap stops a long hitch from simulating a burst of steps.
	const float Step = 1.f / SimRate;
	Accumulator = FMath::Min(Accumulator + DeltaSeconds, Step * 8.f);
	while (bInPlay && Accumulator >= Step)
	{
		PreviousPosition = Position;
		Substep(Step);
		Accumulator -= Step;
	}

	// Draw part-way between the last two substeps, by how far we are into the next one.
	UpdateDrawnTransform(bInPlay ? Accumulator / Step : 1.f);
}

void AIJPBall::Substep(float StepSeconds)
{
	float Remaining = StepSeconds;
	for (int32 Bounce = 0; Bounce < MaxBouncesPerStep && Remaining > 0.f && bInPlay; ++Bounce)
	{
		const FVector2D Target = Position + Velocity * Remaining;
		FHitResult Hit;
		if (!Sweep(Position, Target, Hit))
		{
			Position = Target;
			return;
		}

		if (Hit.bStartPenetrating && !Cast<UIJPGoalComponent>(Hit.GetComponent()))
		{
			// Something moved onto the ball (e.g. a paddle sliding over it). Push out and retry; no time passes.
			Position += Arena->WorldDirToPlane(Hit.Normal) * (Hit.PenetrationDepth + ContactSkin);
			continue;
		}

		// Hit.Location is where the box stopped. Step off the surface a little more, because a sweep that
		// starts touching a surface can report it again even while moving away.
		Position = Arena->WorldToPlane(Hit.Location) + Arena->WorldDirToPlane(Hit.Normal) * ContactSkin;
		Remaining *= 1.f - Hit.Time;
		HandleHit(Hit);
	}
}

bool AIJPBall::Sweep(const FVector2D& From, const FVector2D& To, FHitResult& OutHit) const
{
	const AIJPArena* ArenaPtr = Arena.Get();
	const float Half = Size * 0.5f;
	const FCollisionShape Box = FCollisionShape::MakeBox(FVector(Half, Half, Half));
	const FCollisionQueryParams Params(SCENE_QUERY_STAT(IJPBallSweep), false, this);

	// The box is aligned with the arena, so ball faces meet wall and paddle faces flat.
	return GetWorld()->SweepSingleByChannel(OutHit, ArenaPtr->PlaneToWorld(From), ArenaPtr->PlaneToWorld(To),
		ArenaPtr->GetActorQuat(), ECC_PongBall, Box, Params);
}

void AIJPBall::HandleHit(const FHitResult& Hit)
{
	if (const UIJPGoalComponent* Goal = Cast<UIJPGoalComponent>(Hit.GetComponent()))
	{
		bInPlay = false;
		Velocity = FVector2D::ZeroVector;
		SetActorHiddenInGame(true);
		OnGoal.Broadcast(Goal->DefendingSide);
		return;
	}

	const FVector2D Normal = Arena->WorldDirToPlane(Hit.Normal).GetSafeNormal();

	// Already leaving this surface (a touch reported right after bouncing off it): not a new contact.
	// Reflecting here would turn the ball back into the surface and bounce it again.
	if (FVector2D::DotProduct(Velocity, Normal) >= 0.f)
	{
		return;
	}

	if (AIJPPaddle* Paddle = Cast<AIJPPaddle>(Hit.GetActor()))
	{
		if (TryPaddleBounce(Paddle, Normal))
		{
			return;
		}
	}

	// Walls, and the top/bottom edges of paddles: a plain mirror bounce.
	Velocity = FIJPPongMath::ClampAngle(FIJPPongMath::Reflect(Velocity, Normal), MaxBounceAngleDeg);
	OnBounce.Broadcast();
}

bool AIJPBall::TryPaddleBounce(AIJPPaddle* Paddle, const FVector2D& Normal)
{
	// Only the face pointing at the net counts, and only while the ball is heading toward that paddle's goal.
	const float GoalDir = IJP::SideSign(Paddle->GetSide());
	const bool bFrontFace = FMath::Abs(Normal.X) > FMath::Abs(Normal.Y) && Normal.X * GoalDir < 0.f;
	if (!bFrontFace || Velocity.X * GoalDir <= 0.f)
	{
		return false;
	}

	// Where on the paddle it hit: -1 bottom edge .. +1 top edge, counting the ball's own half-size.
	const float Reach = Paddle->GetSize().Y * 0.5f + Size * 0.5f;
	const float Offset = (Position.Y - Paddle->GetPlanePosition().Y) / Reach;

	Speed = FMath::Min(Speed + SpeedPerHit, MaxSpeed);
	Velocity = FIJPPongMath::ComputePaddleBounce(Offset, Speed, MaxBounceAngleDeg, -GoalDir);
	++RallyHits;
	OnPaddleHit.Broadcast(Paddle);
	return true;
}

void AIJPBall::UpdateDrawnTransform(float Alpha)
{
	const AIJPArena* ArenaPtr = Arena.Get();
	const FVector2D Drawn = FMath::Lerp(PreviousPosition, Position, Alpha);
	SetActorLocationAndRotation(ArenaPtr->PlaneToWorld(Drawn), ArenaPtr->GetActorQuat());
}
