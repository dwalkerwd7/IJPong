// It's Just Pong

#include "Gameplay/IJPSpellStrike.h"
#include "Components/StaticMeshComponent.h"
#include "Core/IJPGameModeBase.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Gameplay/IJPArena.h"
#include "Gameplay/IJPMatchComponent.h"
#include "Gameplay/IJPPaddle.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	constexpr float StrikeCubeSize = 100.f; // /Engine/BasicShapes/Cube and Plane are 100 units, centred.
	constexpr float StrikeDepth = 4.f;       // just in front of play
	constexpr float StrikeSpriteDepth = 5.f; // sprites on top of it
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

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Plane(TEXT("/Engine/BasicShapes/Plane.Plane"));
	for (TObjectPtr<UStaticMeshComponent>* Quad : { &ShotQuad, &MarkerQuad, &ImpactQuad })
	{
		*Quad = MakePiece(Quad == &ShotQuad ? TEXT("ShotQuad") : Quad == &MarkerQuad ? TEXT("MarkerQuad") : TEXT("ImpactQuad"));
		(*Quad)->SetStaticMesh(Plane.Object);
		(*Quad)->SetRelativeRotation(IJP::SpriteQuadRotation);
		(*Quad)->SetVisibility(false);
	}
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
	const EIJPPaletteRole CasterRole = InCasterSide == EIJPSide::Left ? EIJPPaletteRole::LeftPaddle : EIJPPaletteRole::RightPaddle;
	UMaterialInterface* Colour = InArena->GetPaletteMaterial(CasterRole);
	for (UStaticMeshComponent* Piece : { MarkerTop.Get(), MarkerBottom.Get(), Shot.Get() })
	{
		Piece->SetMaterial(0, Colour);
	}
	const FLinearColor Tint = InArena->GetPalette().Get(CasterRole);
	bShotSprite = PrepareSprite(ShotQuad, Spec.ShotSprite, Tint);
	bMarkerSprite = PrepareSprite(MarkerQuad, Spec.MarkerSprite, Tint);
	bImpactSprite = PrepareSprite(ImpactQuad, Spec.ImpactSprite, Tint);

	Place(MarkerTop, FVector2D(LaneX, TargetY + Spec.HalfHeight), FVector2D(StrikeMarkerWidth, StrikeLineThickness));
	Place(MarkerBottom, FVector2D(LaneX, TargetY - Spec.HalfHeight), FVector2D(StrikeMarkerWidth, StrikeLineThickness));
	if (bMarkerSprite)
	{
		// Its own size, or fitted to the zone's height at the art's own proportions.
		const float Height = Spec.MarkerSize.Y > 0.f ? Spec.MarkerSize.Y : Spec.HalfHeight * 2.f;
		const float Width = Spec.MarkerSize.X > 0.f ? Spec.MarkerSize.X : Height * Spec.MarkerSprite->GetSizeX() / FMath::Max(Spec.MarkerSprite->GetSizeY(), 1);
		Place(MarkerQuad, FVector2D(LaneX, TargetY), FVector2D(Width, Height));
	}
	ShowMarker(true);
	if (Spec.bTravels)
	{
		PlaceShot(FVector2D(StartX, TargetY), bShotSprite ? Spec.ShotSize : FVector2D(StrikeShotSize));
	}
	else if (Spec.bFalls)
	{
		PlaceShot(FVector2D(LaneX, InArena->GetHalfExtents().Y), bShotSprite ? Spec.ShotSize : FVector2D(StrikeShotSize));
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
	ShowMarker(FMath::Frac(Elapsed * BlinkRate) < 0.6f);
	const FVector2D ShotSize = bShotSprite ? Spec.ShotSize : FVector2D(StrikeShotSize);
	if (Spec.bTravels)
	{
		PlaceShot(FVector2D(FMath::Lerp(StartX, LaneX, Progress), TargetY), ShotSize);
	}
	else if (Spec.bFalls)
	{
		// Gathering speed like something dropped.
		PlaceShot(FVector2D(LaneX, FMath::Lerp(Arena->GetHalfExtents().Y, TargetY, Progress * Progress)), ShotSize);
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
	ShowMarker(false);

	if (Spec.bFalls && !Spec.bTravels)
	{
		// It has landed: gone, leaving only the impact.
		Shot->SetVisibility(false);
		ShotQuad->SetVisibility(false);
	}
	else if (!Spec.bTravels)
	{
		// Lightning: a bolt from the top wall down to the zone, for a moment.
		const float Top = Arena->GetHalfExtents().Y;
		PlaceShot(FVector2D(LaneX, (Top + TargetY) * 0.5f), FVector2D(bShotSprite ? Spec.ShotSize.X : StrikeBoltWidth, FMath::Max(Top - TargetY, 1.f)));
	}
	if (bImpactSprite)
	{
		Place(ImpactQuad, FVector2D(LaneX, TargetY), Spec.ImpactSize);
		ImpactQuad->SetVisibility(true);
		Linger = FMath::Max(Linger, Spec.ImpactTime);
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
	if (Piece == ShotQuad || Piece == MarkerQuad || Piece == ImpactQuad)
	{
		// A plane turned to face the camera: its own Y runs up the screen.
		Piece->SetRelativeLocation(FVector(Centre.X, StrikeSpriteDepth, Centre.Y));
		Piece->SetRelativeScale3D(FVector(Size.X / StrikeCubeSize, Size.Y / StrikeCubeSize, 1.f));
		return;
	}
	Piece->SetRelativeLocation(FVector(Centre.X, StrikeDepth, Centre.Y));
	Piece->SetRelativeScale3D(FVector(Size.X / StrikeCubeSize, 1.f / StrikeCubeSize, Size.Y / StrikeCubeSize));
}

void AIJPSpellStrike::PlaceShot(const FVector2D& Centre, const FVector2D& Size)
{
	ShotPosition = Centre;
	UStaticMeshComponent* Piece = bShotSprite ? ShotQuad.Get() : Shot.Get();
	Place(Piece, Centre, Size);
	if (bShotSprite)
	{
		// Drawn heading right: a shot from the right paddle faces left.
		const float Facing = Spec.bTravels && LaneX < StartX ? -1.f : 1.f;
		Piece->SetRelativeScale3D(FVector(Facing * Size.X / StrikeCubeSize, Size.Y / StrikeCubeSize, 1.f));
	}
	Piece->SetVisibility(true);
}

bool AIJPSpellStrike::PrepareSprite(UStaticMeshComponent* Quad, UTexture2D* Sprite, const FLinearColor& Colour)
{
	UMaterialInterface* Base = Sprite && Arena->ShowsSprites() ? Arena->GetSpriteMaterial() : nullptr;
	if (!Base)
	{
		return false;
	}
	UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(Base, this);
	Material->SetTextureParameterValue(TEXT("Sprite"), Sprite);
	Material->SetVectorParameterValue(TEXT("Color"), Colour);
	Quad->SetMaterial(0, Material);
	return true;
}

void AIJPSpellStrike::ShowMarker(bool bShow)
{
	MarkerQuad->SetVisibility(bShow && bMarkerSprite);
	MarkerTop->SetVisibility(bShow && !bMarkerSprite);
	MarkerBottom->SetVisibility(bShow && !bMarkerSprite);
}

bool AIJPSpellStrike::IsShotSpriteShown() const
{
	return ShotQuad->IsVisible();
}

bool AIJPSpellStrike::IsMarkerSpriteShown() const
{
	return MarkerQuad->IsVisible();
}

bool AIJPSpellStrike::IsImpactShown() const
{
	return ImpactQuad->IsVisible();
}

void AIJPSpellStrike::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (Arena.IsValid())
	{
		Arena->UnregisterStrike(this);
	}
	Super::EndPlay(EndPlayReason);
}
