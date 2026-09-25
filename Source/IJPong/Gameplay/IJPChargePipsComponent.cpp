// It's Just Pong

#include "Gameplay/IJPChargePipsComponent.h"
#include "Core/IJPTypes.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

UIJPChargePipsComponent::UIJPChargePipsComponent()
{
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	SetStaticMesh(CubeMesh.Object);
	IJP::ConfigureAsVisualOnly(this);
	CastShadow = false;
}

void UIJPChargePipsComponent::SetCharge(int32 InFilled, int32 InTotal)
{
	InTotal = FMath::Max(InTotal, 0);
	InFilled = FMath::Clamp(InFilled, 0, InTotal);
	if (InFilled == Filled && InTotal == Total && (GetInstanceCount() > 0 || Total == 0))
	{
		return;
	}
	Filled = InFilled;
	Total = InTotal;
	Rebuild();

	// Full = ready: blink until it's spent.
	const bool bFull = Total > 0 && Filled >= Total;
	if (bFull && !Blinker.IsRunning())
	{
		Blinker.Start(this, 0.25f, 0, true, [this](bool bShow) { SetVisibility(bShow); });
	}
	else if (!bFull && Blinker.IsRunning())
	{
		Blinker.Stop();
		SetVisibility(true);
	}
}

void UIJPChargePipsComponent::Rebuild()
{
	ClearInstances();
	const float CubeSize = 100.f; // /Engine/BasicShapes/Cube is 100 units, centred.
	const float Step = PipSize + PipSpacing;
	for (int32 i = 0; i < Total; ++i)
	{
		const float X = (i - (Total - 1) * 0.5f) * Step;
		const float Size = i < Filled ? PipSize : PipSize * EmptyScale;
		AddInstance(FTransform(FQuat::Identity, FVector(X, 0.f, 0.f), FVector(Size / CubeSize, 10.f / CubeSize, Size / CubeSize)));
	}
}
