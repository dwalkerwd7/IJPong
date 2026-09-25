// It's Just Pong

#include "Narrative/IJPPortraitComponent.h"
#include "Core/IJPTypes.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

UIJPPortraitComponent::UIJPPortraitComponent()
{
	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneMesh(TEXT("/Engine/BasicShapes/Plane.Plane"));
	SetStaticMesh(PlaneMesh.Object);
	IJP::ConfigureAsVisualOnly(this);
	CastShadow = false;
	SetVisibility(false);
	SetRelativeRotation(IJP::SpriteQuadRotation);
}

void UIJPPortraitComponent::SetPortraits(const FIJPPortraits& InPortraits)
{
	Portraits = InPortraits;
	LineExpression = EIJPExpression::Mood;
	Refresh();
}

void UIJPPortraitComponent::SetLook(bool bInSpriteEra, UMaterialInterface* SpriteMaterial, const FLinearColor& Colour)
{
	bSpriteEra = bInSpriteEra;
	if (!Material && SpriteMaterial)
	{
		Material = UMaterialInstanceDynamic::Create(SpriteMaterial, this);
		SetMaterial(0, Material);
	}
	if (Material)
	{
		static const FName ColorParam(TEXT("Color"));
		Material->SetVectorParameterValue(ColorParam, Colour);
	}
	const float PlaneSize = 100.f; // /Engine/BasicShapes/Plane is 100 units square.
	SetRelativeScale3D(FVector(Size / PlaneSize, Size / PlaneSize, 1.f));
	Refresh();
}

void UIJPPortraitComponent::SetMood(EIJPExpression InMood)
{
	Mood = InMood == EIJPExpression::Mood ? EIJPExpression::Neutral : InMood;
	Refresh();
}

void UIJPPortraitComponent::SetLineExpression(EIJPExpression InExpression)
{
	LineExpression = InExpression;
	Refresh();
}

void UIJPPortraitComponent::Refresh()
{
	const bool bShow = bSpriteEra && Portraits.IsSet() && Material;
	if (bShow)
	{
		static const FName SpriteParam(TEXT("Sprite"));
		Material->SetTextureParameterValue(SpriteParam, Portraits.Get(GetExpression()));
	}
	SetVisibility(bShow);
}
