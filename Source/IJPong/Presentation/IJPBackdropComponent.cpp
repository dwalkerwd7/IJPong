// It's Just Pong

#include "Presentation/IJPBackdropComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/IJPTypes.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Presentation/IJPBackdrop.h"

namespace
{
	const FName BackdropSpriteParam(TEXT("Sprite"));
	const FName BackdropColorParam(TEXT("Color"));
	const FName BackdropOffsetParam(TEXT("Offset"));
	constexpr float BackdropPlaneSize = 100.f; // /Engine/BasicShapes/Plane is 100 units square.

	// Between the arena's background slab (-10) and the chat bubbles (-8): behind everything else.
	constexpr float BackdropDepth = -9.f;

	// Screen-filling layers are drawn this much bigger, so a shake never shows their edges.
	constexpr float BackdropOverscan = 1.04f;

	// Drawn before any other see-through piece (bubbles, strikes), back layer first.
	constexpr int32 BackdropSortBase = -100;
}

UIJPBackdropComponent::UIJPBackdropComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UIJPBackdropComponent::SetBackdrop(const UIJPBackdrop* InBackdrop, UMaterialInterface* Material, const FVector2D& InScreenSize)
{
	Backdrop = InBackdrop;
	ScreenSize = InScreenSize;
	bBroken = false;
	BurstLeft = 0.f;
	ShakeLeft = 0.f;
	ShakeOffset = FVector2D::ZeroVector;
	Time = 0.f;
	Rebuild(Material);
	SetShown(bShown);
}

void UIJPBackdropComponent::Rebuild(UMaterialInterface* Material)
{
	Pieces.Reset();
	Scroll.Reset();
	if (Backdrop)
	{
		for (int32 Layer = 0; Layer < Backdrop->Layers.Num(); ++Layer)
		{
			const FIJPBackdropLayer& Each = Backdrop->Layers[Layer];
			Scroll.Add(FVector2D::ZeroVector);
			Pieces.Add({ Layer, Each.Centre });
			for (const FVector2D& More : Each.MoreCentres)
			{
				Pieces.Add({ Layer, More });
			}
		}
	}

	while (Quads.Num() < Pieces.Num() && Material)
	{
		UStaticMeshComponent* Quad = NewObject<UStaticMeshComponent>(GetOwner(), *FString::Printf(TEXT("%s_Layer%d"), *GetName(), Quads.Num()));
		Quad->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane")));
		Quad->SetupAttachment(this);
		Quad->SetRelativeRotation(IJP::SpriteQuadRotation);
		Quad->CastShadow = false;
		IJP::ConfigureAsVisualOnly(Quad);
		Quad->RegisterComponent();
		UMaterialInstanceDynamic* Instance = UMaterialInstanceDynamic::Create(Material, this);
		Quad->SetMaterial(0, Instance);
		Quads.Add(Quad);
		Materials.Add(Instance);
	}
	ApplyTextures();
	UpdatePieces();
}

void UIJPBackdropComponent::ApplyTextures()
{
	for (int32 i = 0; i < Pieces.Num() && i < Quads.Num(); ++i)
	{
		Materials[i]->SetTextureParameterValue(BackdropSpriteParam, const_cast<UTexture2D*>(GetShownTexture(Pieces[i].Layer)));
		Quads[i]->SetTranslucentSortPriority(BackdropSortBase + Pieces[i].Layer);
	}
}

const UTexture2D* UIJPBackdropComponent::GetShownTexture(int32 Index) const
{
	if (!Backdrop || !Backdrop->Layers.IsValidIndex(Index))
	{
		return nullptr;
	}
	const FIJPBackdropLayer& Layer = Backdrop->Layers[Index];
	return bBroken && Layer.BrokenTexture ? Layer.BrokenTexture.Get() : Layer.Texture.Get();
}

void UIJPBackdropComponent::SetShown(bool bInShown)
{
	bShown = bInShown;
	const bool bDraw = IsShown();
	for (int32 i = 0; i < Quads.Num(); ++i)
	{
		Quads[i]->SetVisibility(bDraw && i < Pieces.Num() && GetShownTexture(Pieces[i].Layer));
	}
	SetComponentTickEnabled(bDraw);
}

void UIJPBackdropComponent::Shake(float Strength)
{
	ShakeStrength = FMath::Max(ShakeLeft > 0.f ? ShakeStrength : 0.f, Strength);
	ShakeLeft = ShakeTime;
}

void UIJPBackdropComponent::Break()
{
	if (!Backdrop || bBroken)
	{
		return;
	}
	bBroken = true;
	BurstLeft = BurstTime;
	ApplyTextures();
	SetShown(bShown);
	Shake(BreakShake);
}

void UIJPBackdropComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!Backdrop)
	{
		return;
	}
	Time += DeltaTime;

	const bool bBursting = BurstLeft > 0.f;
	BurstLeft = FMath::Max(BurstLeft - DeltaTime, 0.f);
	for (int32 Layer = 0; Layer < Scroll.Num(); ++Layer)
	{
		const FIJPBackdropLayer& Each = Backdrop->Layers[Layer];
		Scroll[Layer] += Each.Drift * DeltaTime * (bBursting ? Each.BreakBurst : 1.f);
	}

	// A shake jitters, fading out over its time.
	if (ShakeLeft > 0.f)
	{
		ShakeLeft = FMath::Max(ShakeLeft - DeltaTime, 0.f);
		const float Fade = ShakeTime > 0.f ? ShakeLeft / ShakeTime : 0.f;
		ShakeOffset = FVector2D(FMath::FRandRange(-1.f, 1.f), FMath::FRandRange(-1.f, 1.f)) * ShakeAmount * ShakeStrength * Fade;
	}
	else
	{
		ShakeOffset = FVector2D::ZeroVector;
	}
	UpdatePieces();
}

void UIJPBackdropComponent::UpdatePieces()
{
	if (!Backdrop)
	{
		return;
	}
	for (int32 i = 0; i < Pieces.Num() && i < Quads.Num(); ++i)
	{
		const FIJPBackdropLayer& Layer = Backdrop->Layers[Pieces[i].Layer];
		const bool bFillsScreen = Layer.Size.X >= 1.f && Layer.Size.Y >= 1.f;
		const FVector2D Size = Layer.Size * ScreenSize * (bFillsScreen ? BackdropOverscan : 1.f);
		const FVector2D Shaken = ShakeOffset * Layer.ShakeScale;
		const FVector2D& Centre = Pieces[i].Centre;
		Quads[i]->SetRelativeLocation(FVector((Centre.X - 0.5f) * ScreenSize.X + Shaken.X, BackdropDepth, (0.5f - Centre.Y) * ScreenSize.Y + Shaken.Y));
		Quads[i]->SetRelativeScale3D(FVector(Size.X / BackdropPlaneSize, Size.Y / BackdropPlaneSize, 1.f));

		// The picture scrolls inside its quad: moving it right samples further left, moving it up samples further down.
		const FVector2D Scrolled = Scroll[Pieces[i].Layer];
		const FVector2D SafeSize(FMath::Max(Layer.Size.X, 0.01f), FMath::Max(Layer.Size.Y, 0.01f));
		Materials[i]->SetVectorParameterValue(BackdropOffsetParam, FLinearColor(FMath::Frac(-Scrolled.X / SafeSize.X), FMath::Frac(Scrolled.Y / SafeSize.Y), 0.f, 0.f));

		// A wavering flicker (two sines, each sprite out of step with the others).
		float Brightness = 1.f;
		if (Layer.Flicker > 0.f)
		{
			const float Phase = Time * Layer.FlickerSpeed * UE_TWO_PI + i * 1.7f;
			const float Waver = FMath::Clamp(0.5f + 0.3f * FMath::Sin(Phase) + 0.2f * FMath::Sin(Phase * 2.3f + 0.9f), 0.f, 1.f);
			Brightness = 1.f - Layer.Flicker * Waver;
		}
		FLinearColor Colour = Layer.Tint * Brightness;
		Colour.A = Layer.Tint.A;
		Materials[i]->SetVectorParameterValue(BackdropColorParam, Colour);
	}
}
