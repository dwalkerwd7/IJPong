// It's Just Pong

#include "Run/IJPRunMapView.h"
#include "Camera/CameraComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Gameplay/IJPArena.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Presentation/IJPCRTComponent.h"
#include "Run/IJPActConfig.h"
#include "Run/IJPRunSubsystem.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	constexpr float CubeSize = 100.f; // /Engine/BasicShapes/Cube is 100 units, centred.

	// Depth toward the camera: the screen, the drawing, the letters.
	constexpr float BackgroundDepth = -10.f;
	constexpr float TextDepth = 2.f;

	const TCHAR* Glyph(EIJPNodeType Type)
	{
		switch (Type)
		{
		case EIJPNodeType::Match: return TEXT("M");
		case EIJPNodeType::Elite: return TEXT("E");
		case EIJPNodeType::Rest:  return TEXT("R");
		case EIJPNodeType::Shop:  return TEXT("$");
		case EIJPNodeType::Event: return TEXT("?");
		case EIJPNodeType::Boss:  return TEXT("B");
		default:                  return TEXT(" ");
		}
	}

	// Brightness of the three states, as linear fractions of the palette's score colour (the screen's gamma
	// lifts them a lot: 0.1 shows at roughly a third).
	constexpr float MidScale = 0.32f;
	constexpr float DimScale = 0.1f;

	FLinearColor Scaled(const FLinearColor& Colour, float Scale)
	{
		return FLinearColor(Colour.R * Scale, Colour.G * Scale, Colour.B * Scale, 1.f);
	}
}

AIJPRunMapView::AIJPRunMapView()
{
	PrimaryActorTick.bCanEverTick = false;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	// Same framing as the arena's camera: plane X right, plane Y (local Z) up, looking down -Y.
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(Root);
	Camera->SetRelativeLocationAndRotation(FVector(0.f, 1000.f, 0.f), FRotator(0.f, -90.f, 0.f));
	Camera->ProjectionMode = ECameraProjectionMode::Orthographic;
	Camera->bConstrainAspectRatio = true;
	Camera->AspectRatio = 4.f / 3.f;

	CRT = CreateDefaultSubobject<UIJPCRTComponent>(TEXT("CRT"));

	Background = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Background"));
	Background->SetupAttachment(Root);
	Background->SetStaticMesh(CubeMesh.Object);
	Background->CastShadow = false;
	IJP::ConfigureAsVisualOnly(Background);

	auto MakePieces = [&](const TCHAR* Name)
	{
		UInstancedStaticMeshComponent* Pieces = CreateDefaultSubobject<UInstancedStaticMeshComponent>(Name);
		Pieces->SetupAttachment(Root);
		Pieces->SetStaticMesh(CubeMesh.Object);
		Pieces->CastShadow = false;
		IJP::ConfigureAsVisualOnly(Pieces);
		return Pieces;
	};
	BrightPieces = MakePieces(TEXT("BrightPieces"));
	MidPieces = MakePieces(TEXT("MidPieces"));
	DimPieces = MakePieces(TEXT("DimPieces"));
	Cursor = MakePieces(TEXT("Cursor"));

	Header = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Header"));
	Footer = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Footer"));
	for (UTextRenderComponent* Text : { Header.Get(), Footer.Get() })
	{
		Text->SetupAttachment(Root);
		Text->SetHorizontalAlignment(EHTA_Center);
		Text->SetVerticalAlignment(EVRTA_TextCenter);
		// Text faces its +X; turn it to face the camera, which looks down -Y from +Y.
		Text->SetRelativeRotation(FRotator(0.f, 90.f, 0.f));
		Text->CastShadow = false;
		IJP::ConfigureAsVisualOnly(Text);
	}
}

void AIJPRunMapView::Init(AIJPArena* InArena)
{
	Arena = InArena;
	if (!InArena)
	{
		return;
	}

	// The same screen as the arena's: its playfield plus walls and margin, at 4:3.
	HalfScreen.Y = InArena->GetHalfExtents().Y + 20.f;
	HalfScreen.X = HalfScreen.Y * Camera->AspectRatio;
	Camera->OrthoWidth = HalfScreen.X * 2.f;

	// The arena camera's flat, fixed-exposure settings, but keep our own CRT pass.
	const FWeightedBlendables OurBlendables = Camera->PostProcessSettings.WeightedBlendables;
	Camera->PostProcessSettings = InArena->GetCamera()->PostProcessSettings;
	Camera->PostProcessSettings.WeightedBlendables = OurBlendables;

	Background->SetMaterial(0, InArena->GetPaletteMaterial(EIJPPaletteRole::Background));
	Background->SetRelativeLocation(FVector(0.f, BackgroundDepth, 0.f));
	Background->SetRelativeScale3D(FVector(HalfScreen.X * 2.f / CubeSize, 1.f / CubeSize, HalfScreen.Y * 2.f / CubeSize));

	if (UMaterialInterface* Base = InArena->GetBaseMaterial())
	{
		BrightMaterial = UMaterialInstanceDynamic::Create(Base, this);
		MidMaterial = UMaterialInstanceDynamic::Create(Base, this);
		DimMaterial = UMaterialInstanceDynamic::Create(Base, this);
		BrightPieces->SetMaterial(0, BrightMaterial);
		Cursor->SetMaterial(0, BrightMaterial);
		MidPieces->SetMaterial(0, MidMaterial);
		DimPieces->SetMaterial(0, DimMaterial);
	}

	for (UTextRenderComponent* Text : { Header.Get(), Footer.Get() })
	{
		Text->SetWorldSize(TextSize);
		if (UMaterialInterface* TextMaterial = InArena->GetTextMaterial())
		{
			Text->SetTextMaterial(TextMaterial);
		}
	}
	Header->SetRelativeLocation(FVector(0.f, TextDepth, HalfScreen.Y - 28.f));
	Footer->SetRelativeLocation(FVector(0.f, TextDepth, -HalfScreen.Y + 24.f));

	CursorBlinker.Start(this, 0.25f, 0, true, [this](bool bShow) { Cursor->SetVisibility(bShow && !Reachable.IsEmpty()); });
	Refresh();
}

void AIJPRunMapView::Refresh()
{
	const UIJPRunSubsystem* Run = UIJPRunSubsystem::Get(this);
	if (!Run || !Arena.IsValid())
	{
		return;
	}
	ApplyColours();

	const FIJPRunMap& Map = Run->GetMap();
	Reachable = Run->GetReachableNodes();
	Selected = FMath::Clamp(Selected, 0, FMath::Max(Reachable.Num() - 1, 0));

	for (UInstancedStaticMeshComponent* Pieces : { BrightPieces.Get(), MidPieces.Get(), DimPieces.Get() })
	{
		Pieces->ClearInstances();
	}

	// Paths first, so frames sit on top. A path already travelled is brighter.
	for (int32 i = 0; i < Map.Nodes.Num(); ++i)
	{
		for (const int32 Next : Map.Nodes[i].Next)
		{
			const bool bTravelled = Run->IsVisited(i) && Run->IsVisited(Next);
			AddDashes(bTravelled ? MidPieces : DimPieces, NodePosition(i), NodePosition(Next));
		}
	}

	const FIJPPalette& Palette = Arena->GetPalette();
	while (Glyphs.Num() < Map.Nodes.Num())
	{
		Glyphs.Add(MakeText(GlyphSize));
	}
	for (int32 i = 0; i < Glyphs.Num(); ++i)
	{
		UTextRenderComponent* Text = Glyphs[i];
		if (!Map.Nodes.IsValidIndex(i))
		{
			Text->SetVisibility(false);
			continue;
		}

		// Bright: you can go there. Mid: been there. Dim: everything else.
		const FIJPMapNode& Node = Map.Nodes[i];
		const bool bReachable = Reachable.Contains(i);
		const bool bVisited = Run->IsVisited(i);
		UInstancedStaticMeshComponent* Pieces = bReachable ? BrightPieces : bVisited ? MidPieces : DimPieces;
		const float Scale = bReachable ? 1.f : bVisited ? MidScale : DimScale;

		const FVector2D Position = NodePosition(i);
		AddFrame(Pieces, Position, Node.Type == EIJPNodeType::Boss ? NodeSize * 1.4f : NodeSize);
		Text->SetText(FText::FromString(Glyph(Node.Type)));
		Text->SetTextRenderColor(Scaled(Palette.Score, Scale).ToFColor(true));
		Text->SetRelativeLocation(FVector(Position.X, TextDepth, Position.Y));
		Text->SetVisibility(true);
	}

	FString ActName = Run->GetAct() ? Run->GetAct()->DisplayName.ToString().ToUpper() : FString();
	Header->SetText(FText::FromString(FString::Printf(TEXT("%s    HP %d/%d    COINS %d"), *ActName, Run->GetHealth(), Run->GetMaxHealth(), Run->GetCoins())));
	PlaceCursor();
}

void AIJPRunMapView::Step(int32 Direction)
{
	if (!Reachable.IsEmpty())
	{
		Selected = FMath::Clamp(Selected + Direction, 0, Reachable.Num() - 1);
		PlaceCursor();
	}
}

int32 AIJPRunMapView::GetSelectedNode() const
{
	return Reachable.IsValidIndex(Selected) ? Reachable[Selected] : INDEX_NONE;
}

void AIJPRunMapView::SetFooter(const FString& Text)
{
	Footer->SetText(FText::FromString(Text));
}

FVector2D AIJPRunMapView::NodePosition(int32 Node) const
{
	const UIJPRunSubsystem* Run = UIJPRunSubsystem::Get(this);
	const FIJPRunMap& Map = Run->GetMap();
	const FIJPMapNode& MapNode = Map.Nodes[Node];

	// Rows from just under the header down to just above the footer; lanes spread around the middle.
	const float Top = HalfScreen.Y - 80.f;
	const float Bottom = -HalfScreen.Y + 72.f;
	const float RowStep = Map.Rows > 1 ? (Top - Bottom) / (Map.Rows - 1) : 0.f;
	const float LaneStep = Map.Lanes > 1 ? FMath::Min(150.f, (HalfScreen.X * 2.f - 200.f) / (Map.Lanes - 1)) : 0.f;
	const float X = Node == Map.BossIndex ? 0.f : (MapNode.Lane - (Map.Lanes - 1) * 0.5f) * LaneStep;
	return FVector2D(X, Top - MapNode.Row * RowStep);
}

void AIJPRunMapView::AddFrame(UInstancedStaticMeshComponent* Target, const FVector2D& Centre, float Size) const
{
	const float Half = Size * 0.5f;
	const float T = LineThickness;
	auto Bar = [&](float X, float Z, float Width, float Height)
	{
		Target->AddInstance(FTransform(FQuat::Identity, FVector(Centre.X + X, 0.f, Centre.Y + Z), FVector(Width / CubeSize, 1.f / CubeSize, Height / CubeSize)));
	};
	Bar(0.f, Half, Size + T, T);
	Bar(0.f, -Half, Size + T, T);
	Bar(-Half, 0.f, T, Size);
	Bar(Half, 0.f, T, Size);
}

void AIJPRunMapView::AddDashes(UInstancedStaticMeshComponent* Target, const FVector2D& From, const FVector2D& To) const
{
	// A dotted line between the frames, like the net.
	const FVector2D Delta = To - From;
	const float Length = Delta.Size();
	const float Clearance = NodeSize * 0.5f + 6.f;
	if (Length <= Clearance * 2.f)
	{
		return;
	}
	const FVector2D Direction = Delta / Length;
	const FRotator Tilt(FMath::RadiansToDegrees(FMath::Atan2(Direction.Y, Direction.X)), 0.f, 0.f);
	constexpr float Dash = 6.f;
	constexpr float Gap = 6.f;
	for (float Along = Clearance + Dash * 0.5f; Along <= Length - Clearance - Dash * 0.5f; Along += Dash + Gap)
	{
		const FVector2D Point = From + Direction * Along;
		Target->AddInstance(FTransform(Tilt, FVector(Point.X, 0.f, Point.Y), FVector(Dash / CubeSize, 1.f / CubeSize, LineThickness / CubeSize)));
	}
}

UTextRenderComponent* AIJPRunMapView::MakeText(float Size)
{
	UTextRenderComponent* Text = NewObject<UTextRenderComponent>(this);
	Text->SetupAttachment(Root);
	Text->SetHorizontalAlignment(EHTA_Center);
	Text->SetVerticalAlignment(EVRTA_TextCenter);
	Text->SetRelativeRotation(FRotator(0.f, 90.f, 0.f));
	Text->SetWorldSize(Size);
	Text->CastShadow = false;
	IJP::ConfigureAsVisualOnly(Text);
	if (Arena.IsValid())
	{
		if (UMaterialInterface* TextMaterial = Arena->GetTextMaterial())
		{
			Text->SetTextMaterial(TextMaterial);
		}
	}
	Text->RegisterComponent();
	return Text;
}

void AIJPRunMapView::ApplyColours()
{
	// Everything on the map is drawn in the palette's score colour at three brightnesses.
	static const FName ColorParam(TEXT("Color"));
	const FLinearColor Ink = Arena->GetPalette().Score;
	if (BrightMaterial)
	{
		BrightMaterial->SetVectorParameterValue(ColorParam, Scaled(Ink, 1.f));
		MidMaterial->SetVectorParameterValue(ColorParam, Scaled(Ink, MidScale));
		DimMaterial->SetVectorParameterValue(ColorParam, Scaled(Ink, DimScale));
	}
	Header->SetTextRenderColor(Ink.ToFColor(true));
	Footer->SetTextRenderColor(Ink.ToFColor(true));
}

void AIJPRunMapView::PlaceCursor()
{
	Cursor->ClearInstances();
	const int32 Node = GetSelectedNode();
	if (Node != INDEX_NONE)
	{
		AddFrame(Cursor, NodePosition(Node), NodeSize + 12.f);
	}
	Cursor->SetVisibility(Node != INDEX_NONE);
}
