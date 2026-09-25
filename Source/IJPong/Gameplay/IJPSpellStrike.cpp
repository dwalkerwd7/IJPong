// It's Just Pong

#include "Gameplay/IJPSpellStrike.h"
#include "Components/StaticMeshComponent.h"
#include "Core/IJPGameModeBase.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Gameplay/IJPArena.h"
#include "Gameplay/IJPMatchComponent.h"
#include "Gameplay/IJPPaddle.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	constexpr float StrikeCubeSize = 100.f; // /Engine/BasicShapes/Cube is 100 units, centred.
	constexpr float StrikeDepth = 4.f;       // just in front of play
	constexpr float StrikeLineThickness = 3.f;
	constexpr float StrikeMarkerWidth = 36.f;
	constexpr float StrikeShotSize = 14.f;
	constexpr float StrikeBoltWidth = 4.f;
	constexpr float StrikeLingerTime = 0.15f;
}

AIJPSpellStrike::AIJPSpellStrike()
{
	PrimaryActorTick.bCanEverTick = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	auto MakePiece = [this](const TCHAR* Name)
	{
		UStaticMeshComponent* Piece = CreateDefaultSubobject<UStaticMeshComponent>(Name);
		Piece->SetupAttachment(Root);
		Piece->SetStaticMesh(Cube.Object);
		Piece->CastShadow = false;
		IJP::ConfigureAsVisualOnly(Piece);
		return Piece;
	};
	MarkerTop = MakePiece(TEXT("MarkerTop"));
	MarkerBottom = MakePiece(TEXT("MarkerBottom"));
	Shot = MakePiece(TEXT("Shot"));
	Shot->SetVisibility(false);
}

void AIJPSpellStrike::Launch(AIJPArena* InArena, EIJPSide InCasterSide, EIJPSide InTargetSide, float InY, const FIJPStrikeSpec& InSpec)
{
	Arena = InArena;
	CasterSide = InCasterSide;
	TargetSide = InTargetSide;
	TargetY = InY;
	Spec = InSpec;
	SetActorTransform(InArena->GetActorTransform());

	const AIJPPaddle* Target = InArena->GetPaddle(InTargetSide);
	const AIJPPaddle* Caster = InArena->GetPaddle(InCasterSide);
	LaneX = Target ? Target->GetPlanePosition().X : 0.f;
	StartX = Caster ? Caster->GetPlanePosition().X : 0.f;

	// In the caster's colour.
	UMaterialInterface* Colour = InArena->GetPaletteMaterial(InCasterSide == EIJPSide::Left ? EIJPPaletteRole::LeftPaddle : EIJPPaletteRole::RightPaddle);
	for (UStaticMeshComponent* Piece : { MarkerTop.Get(), MarkerBottom.Get(), Shot.Get() })
	{
		Piece->SetMaterial(0, Colour);
	}
	Place(MarkerTop, FVector2D(LaneX, TargetY + Spec.HalfHeight), FVector2D(StrikeMarkerWidth, StrikeLineThickness));
	Place(MarkerBottom, FVector2D(LaneX, TargetY - Spec.HalfHeight), FVector2D(StrikeMarkerWidth, StrikeLineThickness));
	if (Spec.bTravels)
	{
		Place(Shot, FVector2D(StartX, TargetY), FVector2D(StrikeShotSize, StrikeShotSize));
		Shot->SetVisibility(true);
	}
	InArena->RegisterStrike(this);
}

void AIJPSpellStrike::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!Arena.IsValid())
	{
		Destroy();
		return;
	}

	if (bLanded)
	{
		Linger -= DeltaSeconds;
		if (Linger <= 0.f)
		{
			Destroy();
		}
		return;
	}

	Elapsed += DeltaSeconds;
	const float Progress = FMath::Clamp(Elapsed / Spec.Delay, 0.f, 1.f);

	// The warning blinks faster as it nears.
	const float BlinkRate = FMath::Lerp(4.f, 16.f, Progress);
	const bool bMarkerOn = FMath::Frac(Elapsed * BlinkRate) < 0.6f;
	MarkerTop->SetVisibility(bMarkerOn);
	MarkerBottom->SetVisibility(bMarkerOn);
	if (Spec.bTravels)
	{
		Place(Shot, FVector2D(FMath::Lerp(StartX, LaneX, Progress), TargetY), FVector2D(StrikeShotSize, StrikeShotSize));
	}

	if (Progress >= 1.f)
	{
		Land();
	}
}

void AIJPSpellStrike::Land()
{
	bLanded = true;
	Linger = StrikeLingerTime;
	MarkerTop->SetVisibility(false);
	MarkerBottom->SetVisibility(false);

	// Lightning: a bolt from the top wall down to the zone, for a moment.
	if (!Spec.bTravels)
	{
		const float Top = Arena->GetHalfExtents().Y;
		Place(Shot, FVector2D(LaneX, (Top + TargetY) * 0.5f), FVector2D(StrikeBoltWidth, FMath::Max(Top - TargetY, 1.f)));
		Shot->SetVisibility(true);
	}

	// A hit only if the paddle is still (partly) in the zone.
	const AIJPPaddle* Target = Arena->GetPaddle(TargetSide);
	if (Target)
	{
		const float Gap = FMath::Abs(Target->GetPlanePosition().Y - TargetY);
		bHit = Gap <= Spec.HalfHeight + Target->GetSize().Y * 0.5f;
	}
	const AIJPGameModeBase* GameMode = GetWorld()->GetAuthGameMode<AIJPGameModeBase>();
	if (UIJPMatchComponent* Match = bHit && GameMode ? GameMode->GetMatch() : nullptr)
	{
		Match->ApplyDamage(TargetSide, Spec.Damage);
	}
}

void AIJPSpellStrike::Place(UStaticMeshComponent* Piece, const FVector2D& Centre, const FVector2D& Size) const
{
	// Arena local axes: X = plane X, Z = plane Y, Y = depth toward the camera.
	Piece->SetRelativeLocation(FVector(Centre.X, StrikeDepth, Centre.Y));
	Piece->SetRelativeScale3D(FVector(Size.X / StrikeCubeSize, 1.f / StrikeCubeSize, Size.Y / StrikeCubeSize));
}

void AIJPSpellStrike::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (Arena.IsValid())
	{
		Arena->UnregisterStrike(this);
	}
	Super::EndPlay(EndPlayReason);
}
