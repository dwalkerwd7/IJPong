// It's Just Pong

#include "Gameplay/IJPBall.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Gameplay/IJPArena.h"
#include "Gameplay/IJPBallType.h"
#include "Gameplay/IJPGoalComponent.h"
#include "Gameplay/IJPPaddle.h"
#include "Gameplay/IJPPongMath.h"
#include "Materials/MaterialInstanceDynamic.h"
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
	ApplySize();
}

void AIJPBall::InitBall(AIJPArena* InArena, const UIJPBallType* InType)
{
	check(InArena);
	Arena = InArena;
	if (UMaterialInterface* Base = InArena->GetBaseMaterial())
	{
		ColourMaterial = UMaterialInstanceDynamic::Create(Base, this);
		Visual->SetMaterial(0, ColourMaterial);
	}
	SetType(InType);
	ResetBall();
}

void AIJPBall::SetType(const UIJPBallType* InType)
{
	Type = InType;
	ApplySize();
	RefreshColour();
}

const UIJPBallType& AIJPBall::GetType() const
{
	return Type ? *Type : *GetDefault<UIJPBallType>();
}

void AIJPBall::RefreshColour()
{
	if (ColourMaterial && Arena.IsValid())
	{
		static const FName ColorParam(TEXT("Color"));
		ColourMaterial->SetVectorParameterValue(ColorParam, Arena->GetBallColour(Type));
	}
}

float AIJPBall::GetSize() const
{
	return GetType().Size;
}

float AIJPBall::GetMaxSpeed() const
{
	return GetType().MaxSpeed;
}

void AIJPBall::ApplySize()
{
	const float CubeSize = 100.f; // /Engine/BasicShapes/Cube is 100 units, centred.
	const float Size = GetSize();
	Visual->SetRelativeScale3D(FVector(Size / CubeSize, VisualDepth / CubeSize, Size / CubeSize));
}

void AIJPBall::BlinkAtCentre()
{
	ResetBall();
	ServeBlinker.Start(this, ServeBlinkPeriod, 0, false, [this](bool bShow) { SetActorHiddenInGame(!bShow); });
}

void AIJPBall::Serve(EIJPSide Toward, float AngleDeg)
{
	ServeBlinker.Cancel();

	const float AngleRad = FMath::DegreesToRadians(FMath::Clamp(AngleDeg, -MaxBounceAngleDeg, MaxBounceAngleDeg));

	Position = PreviousPosition = FVector2D::ZeroVector;
	Speed = GetType().BaseSpeed;
	UnboostedSpeed = 0.f;
	AngleLimitDeg = MaxBounceAngleDeg;
	CurveTimeLeft = 0.f;
	FreezeLeft = 0.f;
	Velocity = FVector2D(IJP::SideSign(Toward) * FMath::Cos(AngleRad), FMath::Sin(AngleRad)) * Speed;
	Accumulator = 0.f;
	RallyHits = 0;
	bInPlay = true;

	SetActorHiddenInGame(false);
	UpdateDrawnTransform(1.f);
}

void AIJPBall::Boost(float Multiplier)
{
	if (!bInPlay)
	{
		return;
	}
	if (UnboostedSpeed <= 0.f)
	{
		UnboostedSpeed = Speed;
	}
	// May go past MaxSpeed: the sweep can't tunnel, and it only lasts this one shot.
	Speed *= Multiplier;
	Velocity = Velocity.GetSafeNormal() * Speed;
}

void AIJPBall::Curve(float DegreesPerSecond, float Duration, float BendUp)
{
	if (bInPlay)
	{
		CurveRate = DegreesPerSecond;
		CurveTimeLeft = Duration;
		CurveBend = BendUp >= 0.f ? 1.f : -1.f;
	}
}

void AIJPBall::Freeze(float Seconds)
{
	if (bInPlay)
	{
		FreezeLeft = FMath::Max(FreezeLeft, Seconds);
	}
}

void AIJPBall::Launch(const FVector2D& InPosition, const FVector2D& InVelocity)
{
	ServeBlinker.Cancel();
	Position = PreviousPosition = InPosition;
	Velocity = InVelocity;
	Speed = InVelocity.Size();
	UnboostedSpeed = 0.f;
	CurveTimeLeft = 0.f;
	FreezeLeft = 0.f;
	Accumulator = 0.f;
	RallyHits = 0;
	bInPlay = true;

	SetActorHiddenInGame(false);
	UpdateDrawnTransform(1.f);
}

void AIJPBall::SetPlaneVelocity(const FVector2D& InVelocity)
{
	Velocity = InVelocity;
	Speed = InVelocity.Size();
	UnboostedSpeed = 0.f;
}

void AIJPBall::ResetBall()
{
	ServeBlinker.Cancel();
	FreezeLeft = 0.f;
	TimeScale = 1.f;

	Position = PreviousPosition = FVector2D::ZeroVector;
	Velocity = FVector2D::ZeroVector;
	Speed = 0.f;
	UnboostedSpeed = 0.f;
	AngleLimitDeg = MaxBounceAngleDeg;
	CurveTimeLeft = 0.f;
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

	// Frozen: time stands still for the ball (real time passes for the freeze itself).
	if (FreezeLeft > 0.f)
	{
		FreezeLeft -= DeltaSeconds;
		return;
	}
	DeltaSeconds *= TimeScale;

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
	if (CurveTimeLeft > 0.f)
	{
		// Turn toward the bend. Rotating counter-clockwise lifts a ball moving right and drops one
		// moving left, so the sign depends on which way it's going.
		const float Turn = FMath::DegreesToRadians(CurveRate * StepSeconds) * CurveBend * FMath::Sign(Velocity.X);
		Velocity = FIJPPongMath::ClampAngle(Velocity.GetRotated(FMath::RadiansToDegrees(Turn)), AngleLimitDeg);
		CurveTimeLeft -= StepSeconds;
	}

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
	const float Half = GetSize() * 0.5f;
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
		CurveTimeLeft = 0.f;
		SetActorHiddenInGame(true);
		OnGoal.Broadcast(this, Goal->DefendingSide);
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

	// Walls, and the top/bottom edges of paddles: a plain mirror bounce. A curve mirrors with it.
	if (FMath::Abs(Normal.Y) > FMath::Abs(Normal.X))
	{
		CurveBend = -CurveBend;
	}
	Velocity = FIJPPongMath::ClampAngle(FIJPPongMath::Reflect(Velocity, Normal), AngleLimitDeg);
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
	const float Reach = Paddle->GetSize().Y * 0.5f + GetSize() * 0.5f;
	const float Offset = (Position.Y - Paddle->GetPlanePosition().Y) / Reach;

	CurveTimeLeft = 0.f;
	const float RallySpeed = UnboostedSpeed > 0.f ? UnboostedSpeed : Speed;
	UnboostedSpeed = 0.f;
	Speed = FMath::Min(RallySpeed + GetType().SpeedPerHit, GetType().MaxSpeed);
	AngleLimitDeg = FMath::Min(MaxBounceAngleDeg + Paddle->GetReturnAngleBonus(), 85.f);
	Velocity = FIJPPongMath::ComputePaddleBounce(Offset, Speed, AngleLimitDeg, -GoalDir);
	++RallyHits;
	OnPaddleHit.Broadcast(this, Paddle);
	return true;
}

void AIJPBall::UpdateDrawnTransform(float Alpha)
{
	const AIJPArena* ArenaPtr = Arena.Get();
	const FVector2D Drawn = FMath::Lerp(PreviousPosition, Position, Alpha);
	SetActorLocationAndRotation(ArenaPtr->PlaneToWorld(Drawn), ArenaPtr->GetActorQuat());
}
