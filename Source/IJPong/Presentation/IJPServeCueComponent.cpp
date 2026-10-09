// It's Just Pong

#include "Presentation/IJPServeCueComponent.h"
#include "Audio/IJPToneSet.h"
#include "Audio/IJPToneSynthComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Gameplay/IJPArena.h"
#include "Gameplay/IJPBall.h"

namespace
{
	constexpr float CueCubeSize = 100.f; // /Engine/BasicShapes/Cube is 100 units, centred.
	constexpr float CueDepth = 3.f;      // just in front of the court
	constexpr float CueSegmentLength = 5.f;
	constexpr float CueThickness = 3.f;
}

UIJPServeCueComponent::UIJPServeCueComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UIJPServeCueComponent::Start(AIJPBall* InBall, float Duration)
{
	if (!InBall)
	{
		return;
	}
	if (!Ring)
	{
		Ring = NewObject<UInstancedStaticMeshComponent>(GetOwner(), TEXT("ServeCueRing"));
		Ring->SetupAttachment(this);
		Ring->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
		Ring->CastShadow = false;
		IJP::ConfigureAsVisualOnly(Ring);
		Ring->RegisterComponent();
	}
	if (const AIJPArena* Arena = Cast<AIJPArena>(GetOwner()))
	{
		Ring->SetMaterial(0, Arena->GetPaletteMaterial(EIJPPaletteRole::Score));
	}
	Ball = InBall;
	InBall->WaitAtCentre();
	Elapsed = 0.f;
	FillTime = FMath::Max(Duration - FMath::Min(BlinkTime, Duration * 0.4f), UE_KINDA_SMALL_NUMBER);
	bBlinking = false;
	DrawnLit = -1;
	DrawRing(0);
	SetComponentTickEnabled(true);
}

void UIJPServeCueComponent::Stop()
{
	Ball.Reset();
	if (Ring)
	{
		Ring->ClearInstances();
	}
	DrawnLit = -1;
	SetComponentTickEnabled(false);
}

float UIJPServeCueComponent::GetProgress() const
{
	return IsShowing() ? FMath::Clamp(Elapsed / FillTime, 0.f, 1.f) : 0.f;
}

void UIJPServeCueComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	AIJPBall* Waiting = Ball.Get();
	// Served (or taken off the centre some other way): done.
	if (!Waiting || Waiting->IsInPlay() || !Waiting->IsBlinking())
	{
		Stop();
		return;
	}
	Elapsed += DeltaTime;
	DrawRing(FMath::FloorToInt(GetProgress() * Segments));
	if (!bBlinking && Elapsed >= FillTime)
	{
		// Full: the ball blinks and beeps for the last moment before the serve.
		bBlinking = true;
		Waiting->BlinkAtCentre();
		if (const AIJPArena* Arena = Cast<AIJPArena>(GetOwner()))
		{
			Arena->GetTones()->PlayTone(Arena->GetToneSet().Arm);
		}
	}
}

void UIJPServeCueComponent::DrawRing(int32 Lit)
{
	if (!Ring || Lit == DrawnLit)
	{
		return;
	}
	DrawnLit = Lit;
	Ring->ClearInstances();
	// Clockwise from the top. Arena local axes: X = plane X, Z = plane Y, Y = depth.
	for (int32 i = 0; i < Lit; ++i)
	{
		const float Angle = PI * 0.5f - 2.f * PI * (i + 0.5f) / Segments;
		const FVector Centre(FMath::Cos(Angle) * Radius, CueDepth, FMath::Sin(Angle) * Radius);
		const FRotator Along(FMath::RadiansToDegrees(Angle) + 90.f, 0.f, 0.f);
		Ring->AddInstance(FTransform(Along, Centre, FVector(CueSegmentLength / CueCubeSize, 1.f / CueCubeSize, CueThickness / CueCubeSize)));
	}
}
