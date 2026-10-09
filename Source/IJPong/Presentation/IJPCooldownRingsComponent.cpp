// It's Just Pong

#include "Presentation/IJPCooldownRingsComponent.h"
#include "Abilities/IJPAbilityComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Core/IJPTypes.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Gameplay/IJPArena.h"
#include "Gameplay/IJPPaddle.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace
{
	constexpr float RingCubeSize = 100.f;  // /Engine/BasicShapes/Cube is 100 units, centred.
	constexpr float RingPlaneSize = 100.f; // /Engine/BasicShapes/Plane is 100 units square.
	constexpr float RingThickness = 3.f;
	constexpr float RingLabelDepth = 6.f;  // just in front of the court (the camera looks along -Y)
	constexpr float RingBlinkRate = 8.f;   // flashes per second while a ring blinks
	constexpr float RingEmptyScale = 0.35f; // an unlit segment, as a fraction of a lit one
}

UIJPCooldownRingsComponent::UIJPCooldownRingsComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	Rings[0].Slot = EIJPAbilitySlot::ClassSkill;
	Rings[0].Key = TEXT("SPC");
	Rings[1].Slot = EIJPAbilitySlot::RunAbility;
	Rings[1].Key = TEXT("Z");
}

void UIJPCooldownRingsComponent::SetPaddle(AIJPPaddle* InPaddle)
{
	Paddle = InPaddle;
	EnsurePieces();
	if (const AIJPArena* Arena = Cast<AIJPArena>(GetOwner()))
	{
		Pieces->SetMaterial(0, Arena->GetPaletteMaterial(EIJPPaletteRole::Score));
	}
	DrawnKey.Reset();
	ShownItem.Reset();
	Redraw();
}

void UIJPCooldownRingsComponent::EnsurePieces()
{
	if (Pieces)
	{
		return;
	}
	AActor* Owner = GetOwner();
	const AIJPArena* Arena = Cast<AIJPArena>(Owner);
	Pieces = NewObject<UInstancedStaticMeshComponent>(Owner, TEXT("CooldownRingPieces"));
	Pieces->SetupAttachment(this);
	Pieces->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
	Pieces->CastShadow = false;
	IJP::ConfigureAsVisualOnly(Pieces);
	Pieces->RegisterComponent();

	auto MakeText = [this, Owner, Arena](const TCHAR* Name, float Size)
	{
		UTextRenderComponent* Text = NewObject<UTextRenderComponent>(Owner, Name);
		Text->SetupAttachment(this);
		Text->SetHorizontalAlignment(EHTA_Center);
		Text->SetVerticalAlignment(EVRTA_TextCenter);
		// The arena's plane faces the camera along +Y: turn the text to face it the same way.
		Text->SetRelativeRotation(FRotator(0.f, 90.f, 0.f));
		Text->SetWorldSize(Size);
		Text->CastShadow = false;
		IJP::ConfigureAsVisualOnly(Text);
		if (UMaterialInterface* TextMaterial = Arena ? Arena->GetTextMaterial() : nullptr)
		{
			Text->SetTextMaterial(TextMaterial);
		}
		Text->SetVisibility(false);
		Text->RegisterComponent();
		return Text;
	};
	for (int32 i = 0; i < UE_ARRAY_COUNT(Rings); ++i)
	{
		UTextRenderComponent* Label = MakeText(*FString::Printf(TEXT("CooldownRingLabel%d"), i), 8.f);
		Label->SetText(FText::FromString(Rings[i].Key));
		Label->SetRelativeLocation(FVector((i - 0.5f) * RingSpacing, RingLabelDepth, 0.f));
		Labels.Add(Label);
	}
	const float ItemZ = -(Radius + ItemGap + ItemSize * 0.5f);
	ItemLetter = MakeText(TEXT("ItemLetter"), ItemSize * 0.7f);
	ItemLetter->SetRelativeLocation(FVector(0.f, RingLabelDepth, ItemZ));

	ItemIcon = NewObject<UStaticMeshComponent>(Owner, TEXT("ItemIcon"));
	ItemIcon->SetupAttachment(this);
	ItemIcon->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane")));
	ItemIcon->CastShadow = false;
	IJP::ConfigureAsVisualOnly(ItemIcon);
	ItemIcon->SetRelativeRotation(IJP::SpriteQuadRotation);
	ItemIcon->SetRelativeLocation(FVector(0.f, RingLabelDepth, ItemZ));
	const float IconSize = ItemSize - 6.f;
	ItemIcon->SetRelativeScale3D(FVector(IconSize / RingPlaneSize, IconSize / RingPlaneSize, 1.f));
	if (UMaterialInterface* SpriteMaterial = Arena ? Arena->GetSpriteMaterial() : nullptr)
	{
		ItemIconMaterial = UMaterialInstanceDynamic::Create(SpriteMaterial, this);
		ItemIcon->SetMaterial(0, ItemIconMaterial);
	}
	ItemIcon->SetVisibility(false);
	ItemIcon->RegisterComponent();
}

void UIJPCooldownRingsComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	const AIJPPaddle* Owner = Paddle.Get();
	const UIJPAbilityComponent* Abilities = Owner ? Owner->GetAbilities() : nullptr;
	for (FRing& Ring : Rings)
	{
		const float Before = Ring.Fill;
		Ring.Fill = Abilities && Abilities->GetAbility(Ring.Slot) ? 1.f - Abilities->GetCooldownFraction(Ring.Slot) : -1.f;
		// Back to full from a cooldown: blink to say so.
		if (Before >= 0.f && Before < 1.f && Ring.Fill >= 1.f)
		{
			Ring.BlinkLeft = ReadyBlinkTime;
		}
		Ring.BlinkLeft = FMath::Max(Ring.BlinkLeft - DeltaTime, 0.f);
	}
	Redraw();
}

void UIJPCooldownRingsComponent::Redraw()
{
	if (!Pieces)
	{
		return;
	}
	UpdateItem();

	// Rebuild only when what's lit changes.
	const UWorld* World = GetWorld();
	const float Now = World ? World->GetTimeSeconds() : 0.f;
	FString Key;
	for (const FRing& Ring : Rings)
	{
		const bool bBlinkOff = Ring.BlinkLeft > 0.f && FMath::FloorToInt(Now * RingBlinkRate * 2.f) % 2 == 1;
		const int32 Lit = Ring.Fill < 0.f ? -1 : FMath::FloorToInt(Ring.Fill * Segments + KINDA_SMALL_NUMBER);
		Key += FString::Printf(TEXT("%d%c,"), Lit, bBlinkOff ? TEXT('o') : TEXT('x'));
	}
	Key += IsItemShown() ? TEXT("i") : TEXT("-");
	if (Key == DrawnKey)
	{
		return;
	}
	DrawnKey = Key;

	Pieces->ClearInstances();
	if (const AIJPArena* Arena = Cast<AIJPArena>(GetOwner()))
	{
		// Text takes its colour directly, so follow the era's palette here.
		const FColor Ink = Arena->GetPalette().Get(EIJPPaletteRole::Score).ToFColor(true);
		ItemLetter->SetTextRenderColor(Ink);
		for (UTextRenderComponent* Label : Labels)
		{
			Label->SetTextRenderColor(Ink);
		}
	}
	auto Bar = [this](float X, float Z, float Width, float Height, float RotationDeg = 0.f)
	{
		Pieces->AddInstance(FTransform(FRotator(RotationDeg, 0.f, 0.f), FVector(X, 0.f, Z), FVector(Width / RingCubeSize, 1.f / RingCubeSize, Height / RingCubeSize)));
	};
	const float SegmentLength = 2.f * PI * Radius / Segments * 0.7f;
	for (int32 r = 0; r < UE_ARRAY_COUNT(Rings); ++r)
	{
		const FRing& Ring = Rings[r];
		Labels[r]->SetVisibility(Ring.Fill >= 0.f);
		if (Ring.Fill < 0.f)
		{
			continue;
		}
		const bool bBlinkOff = Ring.BlinkLeft > 0.f && FMath::FloorToInt(Now * RingBlinkRate * 2.f) % 2 == 1;
		const int32 Lit = bBlinkOff ? 0 : FMath::FloorToInt(Ring.Fill * Segments + KINDA_SMALL_NUMBER);
		const float CentreX = (r - 0.5f) * RingSpacing;
		// Clockwise from the top, like the serve cue. Arena local axes: X = plane X, Z = plane Y, Y = depth.
		for (int32 i = 0; i < Segments; ++i)
		{
			const float Angle = PI * 0.5f - 2.f * PI * (i + 0.5f) / Segments;
			const float Scale = i < Lit ? 1.f : RingEmptyScale;
			Bar(CentreX + FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, SegmentLength * Scale, RingThickness * Scale, FMath::RadiansToDegrees(Angle) + 90.f);
		}
	}

	// The item's frame.
	if (IsItemShown())
	{
		const float Z = -(Radius + ItemGap + ItemSize * 0.5f);
		const float Half = ItemSize * 0.5f;
		Bar(0.f, Z + Half, ItemSize + RingThickness * 0.5f, RingThickness * 0.5f);
		Bar(0.f, Z - Half, ItemSize + RingThickness * 0.5f, RingThickness * 0.5f);
		Bar(-Half, Z, RingThickness * 0.5f, ItemSize);
		Bar(Half, Z, RingThickness * 0.5f, ItemSize);
	}
}

void UIJPCooldownRingsComponent::UpdateItem()
{
	const AIJPPaddle* Owner = Paddle.Get();
	const UIJPAbilityComponent* Abilities = Owner ? Owner->GetAbilities() : nullptr;
	const UIJPAbility* Item = Abilities && Abilities->HasItem() ? Abilities->GetAbility(EIJPAbilitySlot::Item) : nullptr;
	const AIJPArena* Arena = Cast<AIJPArena>(GetOwner());
	const bool bSprites = Arena && Arena->ShowsSprites();
	if (Item == ShownItem.Get() && bSprites == bItemIconShown && (Item != nullptr) == (ItemLetter->IsVisible() || ItemIcon->IsVisible()))
	{
		return;
	}
	ShownItem = Item;
	bItemIconShown = bSprites;

	// The icon in sprite eras when the item has one; otherwise its initial.
	UTexture2D* Icon = Item ? Item->GetDefinition()->Icon.LoadSynchronous() : nullptr;
	const bool bIcon = Item && bSprites && Icon && ItemIconMaterial;
	if (bIcon)
	{
		static const FName SpriteParam(TEXT("Sprite"));
		static const FName ColorParam(TEXT("Color"));
		ItemIconMaterial->SetTextureParameterValue(SpriteParam, Icon);
		ItemIconMaterial->SetVectorParameterValue(ColorParam, Arena->GetPalette().Get(EIJPPaletteRole::Score));
	}
	ItemIcon->SetVisibility(bIcon);
	const FString Name = Item ? Item->DisplayName.ToString() : FString();
	ItemLetter->SetText(FText::FromString(Name.Left(1).ToUpper()));
	ItemLetter->SetVisibility(Item && !bIcon);
}

float UIJPCooldownRingsComponent::GetFill(EIJPAbilitySlot Slot) const
{
	for (const FRing& Ring : Rings)
	{
		if (Ring.Slot == Slot)
		{
			return Ring.Fill;
		}
	}
	return -1.f;
}

bool UIJPCooldownRingsComponent::IsBlinking(EIJPAbilitySlot Slot) const
{
	for (const FRing& Ring : Rings)
	{
		if (Ring.Slot == Slot)
		{
			return Ring.BlinkLeft > 0.f;
		}
	}
	return false;
}

bool UIJPCooldownRingsComponent::IsItemShown() const
{
	return ShownItem.IsValid();
}
