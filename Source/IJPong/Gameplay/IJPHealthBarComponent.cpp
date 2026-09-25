// It's Just Pong

#include "Gameplay/IJPHealthBarComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/IJPTypes.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace
{
	const FName BarSpriteParam(TEXT("Sprite"));
	const FName BarColorParam(TEXT("Color"));
	const FName BarCutParam(TEXT("Cut"));
	constexpr float BarPlaneSize = 100.f; // /Engine/BasicShapes/Plane is 100 units square.

	// Toward the camera (+Y): frame at the back, then the trail, then the fill.
	constexpr float BarFrameDepth = 0.f;
	constexpr float BarTrailDepth = 0.5f;
	constexpr float BarFillDepth = 1.f;
}

UIJPHealthBarComponent::UIJPHealthBarComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

UStaticMeshComponent* UIJPHealthBarComponent::MakePiece(const TCHAR* Name, UMaterialInterface* SpriteMaterial, UMaterialInstanceDynamic*& OutMaterial)
{
	// Named after this bar: both sides' bars share an owner.
	UStaticMeshComponent* Piece = NewObject<UStaticMeshComponent>(GetOwner(), *FString::Printf(TEXT("%s_%s"), *GetName(), Name));
	Piece->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane")));
	Piece->CastShadow = false;
	IJP::ConfigureAsVisualOnly(Piece);
	Piece->SetupAttachment(this);
	Piece->RegisterComponent();
	OutMaterial = UMaterialInstanceDynamic::Create(SpriteMaterial, this);
	Piece->SetMaterial(0, OutMaterial);
	return Piece;
}

void UIJPHealthBarComponent::SetStyle(const FIJPHealthBarStyle& InStyle, UMaterialInterface* SpriteMaterial)
{
	Style = InStyle;
	if (Style.IsSet() && SpriteMaterial && !FramePiece)
	{
		UMaterialInstanceDynamic* Made = nullptr;
		FramePiece = MakePiece(TEXT("Frame"), SpriteMaterial, Made);
		FrameMaterial = Made;
		TrailPiece = MakePiece(TEXT("Trail"), SpriteMaterial, Made);
		TrailMaterial = Made;
		FillPiece = MakePiece(TEXT("Fill"), SpriteMaterial, Made);
		FillMaterial = Made;
	}
	if (FramePiece && Style.IsSet())
	{
		FrameMaterial->SetTextureParameterValue(BarSpriteParam, Style.Frame);
		TrailMaterial->SetTextureParameterValue(BarSpriteParam, Style.Fill);
		FillMaterial->SetTextureParameterValue(BarSpriteParam, Style.Fill);
		Layout();
		UpdateCuts();
	}
	ApplyVisibility(true);
}

void UIJPHealthBarComponent::SetColours(const FLinearColor& FrameColour, const FLinearColor& FillColour)
{
	if (!FramePiece)
	{
		return;
	}
	FrameMaterial->SetVectorParameterValue(BarColorParam, FrameColour);
	FillMaterial->SetVectorParameterValue(BarColorParam, FillColour);
	FLinearColor Dim = FillColour * TrailDim;
	Dim.A = 1.f;
	TrailMaterial->SetVectorParameterValue(BarColorParam, Dim);
}

FVector2D UIJPHealthBarComponent::GetSize() const
{
	if (!Style.IsSet())
	{
		return FVector2D::ZeroVector;
	}
	return FVector2D(Style.Frame->GetSizeX(), Style.Frame->GetSizeY()) / Style.PixelsPerUnit;
}

void UIJPHealthBarComponent::Layout()
{
	// Everything in texture pixels from the frame's top-left, turned into units from the origin
	// (outer edge, vertical centre; X right, Z up).
	const float Scale = 1.f / Style.PixelsPerUnit;
	const FVector2D FrameSize = GetSize();
	FramePiece->SetRelativeLocationAndRotation(FVector(FrameSize.X * 0.5f, BarFrameDepth, 0.f), FRotator(0.f, 0.f, -90.f));
	FramePiece->SetRelativeScale3D(FVector(FrameSize.X / BarPlaneSize, FrameSize.Y / BarPlaneSize, 1.f));

	const FVector2D FillSize = FVector2D(Style.Fill->GetSizeX(), Style.Fill->GetSizeY()) * Scale;
	const FVector2D FillCorner = Style.FillOffset * Scale;
	const float FillX = FillCorner.X + FillSize.X * 0.5f;
	const float FillZ = FrameSize.Y * 0.5f - FillCorner.Y - FillSize.Y * 0.5f;
	for (UStaticMeshComponent* Piece : { TrailPiece.Get(), FillPiece.Get() })
	{
		Piece->SetRelativeLocationAndRotation(FVector(FillX, Piece == FillPiece ? BarFillDepth : BarTrailDepth, FillZ), FRotator(0.f, 0.f, -90.f));
		Piece->SetRelativeScale3D(FVector(FillSize.X / BarPlaneSize, FillSize.Y / BarPlaneSize, 1.f));
	}
}

float UIJPHealthBarComponent::CutFor(float InFraction) const
{
	if (InFraction <= 0.f)
	{
		return -1.f; // Nothing left: not even the texture's first column.
	}
	if (Style.Segments > 0 && Style.SegmentPitch > 0.f && Style.Fill)
	{
		// Any part of a segment left keeps the whole segment lit (a sliver of health still shows).
		const int32 Lit = FMath::Clamp(FMath::CeilToInt(InFraction * Style.Segments - KINDA_SMALL_NUMBER), 1, Style.Segments);
		return Lit * Style.SegmentPitch / Style.Fill->GetSizeX();
	}
	return InFraction;
}

void UIJPHealthBarComponent::UpdateCuts()
{
	if (FillMaterial)
	{
		FillMaterial->SetScalarParameterValue(BarCutParam, CutFor(Fraction));
		TrailMaterial->SetScalarParameterValue(BarCutParam, CutFor(TrailFraction));
	}
}

void UIJPHealthBarComponent::SetFraction(float InFraction, bool bInstant)
{
	InFraction = FMath::Clamp(InFraction, 0.f, 1.f);
	if (bInstant)
	{
		TrailFraction = InFraction;
		SetComponentTickEnabled(false);
	}
	else if (InFraction < Fraction)
	{
		// A drop: the trail keeps what was there (topping up if it was still draining) and waits.
		TrailFraction = FMath::Max(TrailFraction, Fraction);
		TrailWait = TrailHold;
		SetComponentTickEnabled(true);
	}
	Fraction = InFraction;
	if (TrailFraction < Fraction)
	{
		TrailFraction = Fraction; // A heal carries the trail along.
	}
	UpdateCuts();
}

void UIJPHealthBarComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (TrailWait > 0.f)
	{
		TrailWait -= DeltaTime;
		return;
	}
	TrailFraction = FMath::Max(TrailFraction - TrailSpeed * DeltaTime, Fraction);
	UpdateCuts();
	if (TrailFraction <= Fraction)
	{
		SetComponentTickEnabled(false);
	}
}

void UIJPHealthBarComponent::SetShown(bool bInShown)
{
	bShown = bInShown && Style.IsSet() && FramePiece;
	if (!bShown)
	{
		Blinker.Cancel();
	}
	ApplyVisibility(true);
}

void UIJPHealthBarComponent::ApplyVisibility(bool bFlashShow)
{
	for (UStaticMeshComponent* Piece : { FramePiece.Get(), TrailPiece.Get(), FillPiece.Get() })
	{
		if (Piece)
		{
			Piece->SetVisibility(bShown && bFlashShow);
		}
	}
}

void UIJPHealthBarComponent::Flash(int32 NumFlashes, float Period)
{
	if (bShown)
	{
		// Starts hidden; each flash is an off + on (as the seven-segment numbers do).
		Blinker.Start(this, Period, NumFlashes > 0 ? NumFlashes * 2 - 1 : 0, false, [this](bool bShow) { ApplyVisibility(bShow); });
	}
}

void UIJPHealthBarComponent::StopFlash()
{
	Blinker.Cancel();
	ApplyVisibility(true);
}
