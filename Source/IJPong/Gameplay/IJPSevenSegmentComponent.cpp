// It's Just Pong

#include "Gameplay/IJPSevenSegmentComponent.h"
#include "Core/IJPTypes.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	// Bit 0..6 = segments a..g:
	//    a
	//  f   b
	//    g
	//  e   c
	//    d
	constexpr uint8 DigitMasks[10] = { 0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F };

	/** Centre and size of one segment, in a digit's local X/Z. */
	void GetSegmentRect(int32 Segment, const FVector2D& Size, float T, FVector2D& OutCentre, FVector2D& OutSize)
	{
		const float HalfW = Size.X * 0.5f;
		const float HalfH = Size.Y * 0.5f;
		const FVector2D Horizontal(Size.X, T);
		const FVector2D Vertical(T, HalfH);

		switch (Segment)
		{
		case 0: OutCentre = FVector2D(0.f, HalfH - T * 0.5f);				OutSize = Horizontal; break; // a
		case 1: OutCentre = FVector2D(HalfW - T * 0.5f, HalfH * 0.5f);		OutSize = Vertical; break;   // b
		case 2: OutCentre = FVector2D(HalfW - T * 0.5f, -HalfH * 0.5f);	OutSize = Vertical; break;   // c
		case 3: OutCentre = FVector2D(0.f, -HalfH + T * 0.5f);				OutSize = Horizontal; break; // d
		case 4: OutCentre = FVector2D(-HalfW + T * 0.5f, -HalfH * 0.5f);	OutSize = Vertical; break;   // e
		case 5: OutCentre = FVector2D(-HalfW + T * 0.5f, HalfH * 0.5f);	OutSize = Vertical; break;   // f
		default: OutCentre = FVector2D::ZeroVector;							OutSize = Horizontal; break; // g
		}
	}
}

UIJPSevenSegmentComponent::UIJPSevenSegmentComponent()
{
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	SetStaticMesh(CubeMesh.Object);
	IJP::ConfigureAsVisualOnly(this);
	CastShadow = false;
}

void UIJPSevenSegmentComponent::OnRegister()
{
	Super::OnRegister();
	Rebuild();
}

void UIJPSevenSegmentComponent::SetValue(int32 NewValue)
{
	NewValue = FMath::Max(0, NewValue);
	if (NewValue != Value || GetInstanceCount() == 0)
	{
		Value = NewValue;
		Rebuild();
	}
}

void UIJPSevenSegmentComponent::Flash(int32 NumFlashes, float Period)
{
	// Starts hidden; each flash is an off + on, so 2 toggles per flash ends shown.
	const int32 NumToggles = NumFlashes > 0 ? NumFlashes * 2 - 1 : 0;
	Blinker.Start(this, Period, NumToggles, false, [this](bool bShow) { SetVisibility(bShow); });
}

void UIJPSevenSegmentComponent::StopFlash()
{
	if (Blinker.IsRunning())
	{
		Blinker.Stop();
	}
}

void UIJPSevenSegmentComponent::Rebuild()
{
	ClearInstances();

	const FString Digits = FString::FromInt(Value);
	const int32 NumDigits = Digits.Len();
	const float TotalWidth = NumDigits * DigitSize.X + (NumDigits - 1) * DigitSpacing;
	const float CubeSize = 100.f; // /Engine/BasicShapes/Cube is 100 units, centred.

	for (int32 DigitIndex = 0; DigitIndex < NumDigits; ++DigitIndex)
	{
		const uint8 Mask = DigitMasks[Digits[DigitIndex] - TEXT('0')];
		const float DigitCentreX = -TotalWidth * 0.5f + DigitSize.X * 0.5f + DigitIndex * (DigitSize.X + DigitSpacing);

		for (int32 Segment = 0; Segment < 7; ++Segment)
		{
			if ((Mask & (1 << Segment)) == 0)
			{
				continue;
			}

			FVector2D Centre, Size;
			GetSegmentRect(Segment, DigitSize, SegmentThickness, Centre, Size);
			const FVector Location(DigitCentreX + Centre.X, 0.f, Centre.Y);
			const FVector Scale(Size.X / CubeSize, Depth / CubeSize, Size.Y / CubeSize);
			AddInstance(FTransform(FQuat::Identity, Location, Scale));
		}
	}
}
