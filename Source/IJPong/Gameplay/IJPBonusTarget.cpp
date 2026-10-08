// It's Just Pong

#include "Gameplay/IJPBonusTarget.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Gameplay/IJPArena.h"
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

void AIJPBonusTarget::Init(AIJPArena* InArena, const FVector2D& Centre, float Size)
{
	Arena = InArena;
	PlanePosition = Centre;
	// Arena local axes: X = plane X, Z = plane Y, Y = depth toward the camera.
	SetActorTransform(FTransform(InArena->GetActorQuat(), InArena->PlaneToWorld(Centre)));
	Box->SetBoxExtent(FVector(Size * 0.5f, TargetBlockerDepth * 0.5f, Size * 0.5f));
	Visual->SetRelativeScale3D(FVector(Size / TargetCubeSize, TargetVisualDepth / TargetCubeSize, Size / TargetCubeSize));
	Visual->SetMaterial(0, InArena->GetPaletteMaterial(EIJPPaletteRole::Score));
}

void AIJPBonusTarget::Break(AIJPBall* Ball)
{
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
