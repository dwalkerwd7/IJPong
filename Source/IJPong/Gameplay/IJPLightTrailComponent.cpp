// It's Just Pong

#include "Gameplay/IJPLightTrailComponent.h"
#include "CollisionQueryParams.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Era/IJPEra.h"
#include "Era/IJPEraSubsystem.h"
#include "Gameplay/IJPArena.h"
#include "Gameplay/IJPBall.h"

namespace
{
	constexpr float TrailCubeSize = 100.f;     // /Engine/BasicShapes/Cube is 100 units, centred.
	constexpr float TrailBlockerDepth = 200.f; // as deep as the walls, so the ball's sweep always meets it
	constexpr float TrailVisualDepth = 4.f;
	constexpr float TrailDrawDepth = -2.f;     // just behind the balls and paddles
	/** A ball moving within this many degrees of a piece's line slides along it instead of hitting it. */
	constexpr float ParallelDeg = 15.f;
}

UIJPLightTrailComponent::UIJPLightTrailComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UIJPLightTrailComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	const UIJPEra* Era = UIJPEraSubsystem::GetCurrentEra(this);
	const AIJPArena* Arena = GetArena();
	if (!Era || !Arena || !Arena->IsTwistOn(EIJPTwist::LightTrails))
	{
		if (GetNumPieces() > 0 || !Anchors.IsEmpty())
		{
			Reset();
		}
		return;
	}
	const FIJPLightTrails& Settings = Era->LightTrails;

	// Age the pieces: solid after a moment, gone after their life.
	for (FPiece& Piece : Pieces)
	{
		if (!Piece.bActive)
		{
			continue;
		}
		Piece.Age += DeltaTime;
		if (Piece.Age >= Settings.Lifetime)
		{
			Retire(Piece);
		}
		else if (!Piece.bSolid && Piece.Age >= Settings.SolidDelay)
		{
			Piece.bSolid = true;
			Piece.Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		}
	}

	// Lay new pieces behind every ball in play.
	for (AIJPBall* Ball : Arena->GetBalls())
	{
		if (!Ball->IsInPlay() || Ball->IsHeld())
		{
			Anchors.Remove(Ball);
			continue;
		}
		const FVector2D Position = Ball->GetPlanePosition();
		FVector2D* Anchor = Anchors.Find(Ball);
		if (!Anchor)
		{
			Anchors.Add(Ball, Position);
			continue;
		}
		const float Travelled = FVector2D::Distance(*Anchor, Position);
		if (Travelled > Settings.PieceLength * 4.f)
		{
			*Anchor = Position; // it jumped (a serve, a warp): no trail across the gap
		}
		else if (Travelled >= Settings.PieceLength)
		{
			Lay(*Anchor, Position, Settings);
			*Anchor = Position;
		}
	}
}

void UIJPLightTrailComponent::Reset()
{
	for (FPiece& Piece : Pieces)
	{
		Retire(Piece);
	}
	Anchors.Reset();
}

void UIJPLightTrailComponent::AddParallelPieces(const FVector2D& Motion, FCollisionQueryParams& Params) const
{
	const FVector2D Heading = Motion.GetSafeNormal();
	if (Heading.IsZero())
	{
		return;
	}
	const float MinDot = FMath::Cos(FMath::DegreesToRadians(ParallelDeg));
	for (const FPiece& Piece : Pieces)
	{
		if (Piece.bSolid && FMath::Abs(FVector2D::DotProduct(Heading, Piece.Direction)) >= MinDot)
		{
			Params.AddIgnoredComponent(Piece.Box.Get());
		}
	}
}

int32 UIJPLightTrailComponent::GetNumPieces() const
{
	int32 Count = 0;
	for (const FPiece& Piece : Pieces)
	{
		Count += Piece.bActive ? 1 : 0;
	}
	return Count;
}

int32 UIJPLightTrailComponent::GetNumSolidPieces() const
{
	int32 Count = 0;
	for (const FPiece& Piece : Pieces)
	{
		Count += Piece.bActive && Piece.bSolid ? 1 : 0;
	}
	return Count;
}

AIJPArena* UIJPLightTrailComponent::GetArena() const
{
	return Cast<AIJPArena>(GetOwner());
}

void UIJPLightTrailComponent::Lay(const FVector2D& From, const FVector2D& To, const FIJPLightTrails& Settings)
{
	AIJPArena* Arena = GetArena();
	FPiece& Piece = TakePiece();
	const FVector2D Mid = (From + To) * 0.5f;
	const FVector2D Delta = To - From;
	const float Length = Delta.Size() + Settings.Thickness; // overlap the next piece, so the trail has no gaps
	// Arena local axes: X = plane X, Z = plane Y, Y = depth toward the camera. Pitch turns X toward Z.
	const FRotator Rotation(FMath::RadiansToDegrees(FMath::Atan2(Delta.Y, Delta.X)), 0.f, 0.f);
	Piece.Box->SetRelativeLocationAndRotation(FVector(Mid.X, 0.f, Mid.Y), Rotation);
	Piece.Box->SetBoxExtent(FVector(Length * 0.5f, TrailBlockerDepth * 0.5f, Settings.Thickness * 0.5f));
	Piece.Box->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Piece.Visual->SetRelativeLocationAndRotation(FVector(Mid.X, TrailDrawDepth, Mid.Y), Rotation);
	Piece.Visual->SetRelativeScale3D(FVector(Length / TrailCubeSize, TrailVisualDepth / TrailCubeSize, Settings.Thickness / TrailCubeSize));
	Piece.Visual->SetMaterial(0, Arena->GetPaletteMaterial(EIJPPaletteRole::Ball));
	Piece.Visual->SetVisibility(true);
	Piece.Direction = Delta.GetSafeNormal();
	Piece.Age = 0.f;
	Piece.bActive = true;
	Piece.bSolid = false;
}

UIJPLightTrailComponent::FPiece& UIJPLightTrailComponent::TakePiece()
{
	for (FPiece& Piece : Pieces)
	{
		if (!Piece.bActive)
		{
			return Piece;
		}
	}

	// A new pair of components on the arena: a blocker and its look.
	AIJPArena* Arena = GetArena();
	static UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	FPiece& Piece = Pieces.AddDefaulted_GetRef();
	Piece.Box = NewObject<UBoxComponent>(Arena);
	Piece.Box->SetupAttachment(Arena->GetRootComponent());
	IJP::ConfigureAsBallBlocker(Piece.Box);
	Piece.Box->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Piece.Box->RegisterComponent();
	Piece.Visual = NewObject<UStaticMeshComponent>(Arena);
	Piece.Visual->SetupAttachment(Arena->GetRootComponent());
	Piece.Visual->SetStaticMesh(Cube);
	Piece.Visual->CastShadow = false;
	IJP::ConfigureAsVisualOnly(Piece.Visual);
	Piece.Visual->RegisterComponent();
	Owned.Add(Piece.Box);
	Owned.Add(Piece.Visual);
	return Piece;
}

void UIJPLightTrailComponent::Retire(FPiece& Piece)
{
	if (!Piece.bActive)
	{
		return;
	}
	Piece.bActive = false;
	Piece.bSolid = false;
	Piece.Box->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Piece.Visual->SetVisibility(false);
}
