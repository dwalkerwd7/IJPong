// It's Just Pong

#include "Gameplay/IJPBonusTarget.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Audio/IJPToneSet.h"
#include "Audio/IJPToneSynthComponent.h"
#include "Gameplay/IJPArena.h"
#include "Gameplay/IJPBall.h"
#include "Gameplay/IJPBonusTargetComponent.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	constexpr float TargetCubeSize = 100.f;      // /Engine/BasicShapes/Cube is 100 units, centred.
	constexpr float TargetBlockerDepth = 200.f;  // as deep as the walls, so the ball's sweep always meets it
	constexpr float TargetVisualDepth = 10.f;
}

AIJPBonusTarget::AIJPBonusTarget()
{
	Box = CreateDefaultSubobject<UBoxComponent>(TEXT("Box"));
	SetRootComponent(Box);
	IJP::ConfigureAsBallBlocker(Box);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	Visual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Visual"));
	Visual->SetupAttachment(Box);
	Visual->SetStaticMesh(Cube.Object);
	Visual->CastShadow = false;
	IJP::ConfigureAsVisualOnly(Visual);
}

void AIJPBonusTarget::InitBox(AIJPArena* InArena, const FVector2D& Centre, const FVector2D& Size)
{
	Arena = InArena;
	BoxSize = Size;
	// Arena local axes: X = plane X, Z = plane Y, Y = depth toward the camera.
	SetPlanePosition(Centre);
	Box->SetBoxExtent(FVector(Size.X * 0.5f, TargetBlockerDepth * 0.5f, Size.Y * 0.5f));
	Visual->SetRelativeScale3D(FVector(Size.X / TargetCubeSize, TargetVisualDepth / TargetCubeSize, Size.Y / TargetCubeSize));
	Visual->SetMaterial(0, InArena->GetPaletteMaterial(EIJPPaletteRole::Score));
}

void AIJPBonusTarget::SetPlanePosition(const FVector2D& Centre)
{
	PlanePosition = Centre;
	if (const AIJPArena* ArenaPtr = Arena.Get())
	{
		SetActorTransform(FTransform(ArenaPtr->GetActorQuat(), ArenaPtr->PlaneToWorld(Centre)));
	}
}

void AIJPBonusTarget::InitBumper(AIJPArena* InArena, const FVector2D& Centre, const FVector2D& Size, float Boost, EIJPPaletteRole Colour)
{
	InitBox(InArena, Centre, Size);
	bBumper = true;
	BumperBoost = Boost;
	Visual->SetMaterial(0, InArena->GetPaletteMaterial(Colour));
}

void AIJPBonusTarget::Break(AIJPBall* Ball)
{
	if (bBumper)
	{
		// The ball bounces off (the ball does that); a pinball bumper kicks it too.
		if (Ball && BumperBoost > 1.f)
		{
			Ball->Boost(BumperBoost);
			if (AIJPArena* ArenaPtr = Arena.Get())
			{
				ArenaPtr->GetTones()->PlayTone(ArenaPtr->GetToneSet().Pop);
			}
		}
		return;
	}
	if (bBroken)
	{
		return;
	}
	bBroken = true;
	// Out of the way at once: the ball's bounce off it is the last thing it does.
	Box->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetActorHiddenInGame(true);
	if (AIJPArena* ArenaPtr = Arena.Get())
	{
		ArenaPtr->GetBonusTargets()->HandleTargetBroken(this, Ball);
	}
}
