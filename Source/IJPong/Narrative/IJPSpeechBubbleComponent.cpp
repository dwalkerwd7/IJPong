// It's Just Pong

#include "Narrative/IJPSpeechBubbleComponent.h"
#include "Audio/IJPToneSet.h"
#include "Audio/IJPToneSynthComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Gameplay/IJPArena.h"
#include "Gameplay/IJPPaddle.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	constexpr float CubeSize = 100.f; // /Engine/BasicShapes/Cube is 100 units, centred.

	// Depth (toward the camera) of each piece. Paddles and balls fill -5..5 and the arena's
	// background sits at -10, so the bubble lives in between: behind play, in front of the screen.
	constexpr float FillDepth = -8.f;
	constexpr float OutlineDepth = -7.5f;
	constexpr float TextDepth = -7.f;

	// Outline instances: four edges, then the two tail dots.
	constexpr int32 NumEdges = 4;
	constexpr int32 NumTailDots = 2;

	FTransform BoxPiece(float X, float Z, float Width, float Height)
	{
		return FTransform(FQuat::Identity, FVector(X, 0.f, Z), FVector(Width / CubeSize, 1.f / CubeSize, Height / CubeSize));
	}
}

UIJPSpeechBubbleComponent::UIJPSpeechBubbleComponent()
{
	// Ticks only while a line is up.
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	CubeMesh = Cube.Object;
}

void UIJPSpeechBubbleComponent::Say(const FText& Line, float HoldTime)
{
	EnsurePieces();

	FullText = Line.ToString();
	RevealTime = 0.f;
	HoldLeft = HoldTime > 0.f ? HoldTime : BaseHoldTime + HoldPerChar * FullText.Len();
	bTalking = true;

	ApplyLook();
	BuildBox();
	SetShownChars(0);
	FollowPaddle();
	SetVisibility(true, true);
	SetComponentTickEnabled(true);
}

void UIJPSpeechBubbleComponent::Hide()
{
	bTalking = false;
	SetComponentTickEnabled(false);
	SetVisibility(false, true);
}

void UIJPSpeechBubbleComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!bTalking)
	{
		return;
	}

	FollowPaddle();

	if (ShownChars < FullText.Len())
	{
		// Typewriter with babble: a syllable for the last letter typed this frame (one voice, like the cabinet).
		RevealTime += DeltaTime;
		const int32 Target = FMath::Min(FMath::FloorToInt(RevealTime * CharsPerSecond), FullText.Len());
		TCHAR Spoken = 0;
		for (int32 i = ShownChars; i < Target; ++i)
		{
			if (FChar::IsAlnum(FullText[i]))
			{
				Spoken = FullText[i];
			}
		}
		SetShownChars(Target);
		if (Spoken)
		{
			Babble(Spoken);
		}
		return;
	}

	HoldLeft -= DeltaTime;
	if (HoldLeft <= 0.f)
	{
		Finish();
	}
}

AIJPPaddle* UIJPSpeechBubbleComponent::GetPaddle() const
{
	return Cast<AIJPPaddle>(GetOwner());
}

void UIJPSpeechBubbleComponent::EnsurePieces()
{
	if (Text)
	{
		return;
	}

	AActor* Owner = GetOwner();
	Fill = NewObject<UStaticMeshComponent>(Owner, TEXT("BubbleFill"));
	Outline = NewObject<UInstancedStaticMeshComponent>(Owner, TEXT("BubbleOutline"));
	for (UStaticMeshComponent* Piece : TArray<UStaticMeshComponent*>{ Fill, Outline })
	{
		Piece->SetStaticMesh(CubeMesh);
		Piece->CastShadow = false;
		IJP::ConfigureAsVisualOnly(Piece);
		Piece->SetupAttachment(this);
		Piece->RegisterComponent();
	}
	Fill->SetRelativeLocation(FVector(0.f, FillDepth, 0.f));
	Outline->SetRelativeLocation(FVector(0.f, OutlineDepth, 0.f));

	Text = NewObject<UTextRenderComponent>(Owner, TEXT("BubbleText"));
	Text->SetHorizontalAlignment(EHTA_Left);
	Text->SetVerticalAlignment(EVRTA_TextCenter);
	Text->SetWorldSize(TextSize);
	Text->CastShadow = false;
	IJP::ConfigureAsVisualOnly(Text);
	// Text faces its +X; turn it to face the arena camera, which looks down -Y from +Y.
	Text->SetRelativeRotation(FRotator(0.f, 90.f, 0.f));
	Text->SetupAttachment(this);
	Text->RegisterComponent();

	SetVisibility(false, true);
}

void UIJPSpeechBubbleComponent::ApplyLook()
{
	const AIJPPaddle* Paddle = GetPaddle();
	const AIJPArena* Arena = Paddle ? Paddle->GetArena() : nullptr;
	if (!Arena)
	{
		return;
	}

	// The speaker's colour for the frame and text; the screen's background behind the words.
	const bool bLeft = Paddle->GetSide() == EIJPSide::Left;
	Fill->SetMaterial(0, Arena->GetPaletteMaterial(EIJPPaletteRole::Background));
	Outline->SetMaterial(0, Arena->GetPaletteMaterial(bLeft ? EIJPPaletteRole::LeftPaddle : EIJPPaletteRole::RightPaddle));
	if (UMaterialInterface* TextMaterial = Arena->GetTextMaterial())
	{
		Text->SetTextMaterial(TextMaterial);
	}
	const FIJPPalette& Palette = Arena->GetPalette();
	Text->SetTextRenderColor((bLeft ? Palette.LeftPaddle : Palette.RightPaddle).ToFColor(true));
}

void UIJPSpeechBubbleComponent::BuildBox()
{
	// Size the box to the whole line up front, so it doesn't grow while the text types out.
	Text->SetText(FText::FromString(FullText));
	const FVector TextSizeLocal = Text->GetTextLocalSize();
	int32 NumLines = 1;
	for (const TCHAR Char : FullText)
	{
		NumLines += Char == TEXT('\n') ? 1 : 0;
	}
	// Without font metrics (e.g. a headless run) fall back to a rough estimate.
	const float TextWidth = TextSizeLocal.Y > 0.f ? TextSizeLocal.Y : FullText.Len() * TextSize * 0.6f;
	const float TextHeight = TextSizeLocal.Z > 0.f ? TextSizeLocal.Z : NumLines * TextSize;
	BoxSize = FVector2D(TextWidth + Padding * 2.f, TextHeight + Padding * 2.f);

	const float HalfW = BoxSize.X * 0.5f;
	const float HalfH = BoxSize.Y * 0.5f;
	const float T = OutlineThickness;

	Fill->SetRelativeScale3D(FVector(BoxSize.X / CubeSize, 1.f / CubeSize, BoxSize.Y / CubeSize));
	Text->SetRelativeLocation(FVector(-HalfW + Padding, TextDepth, 0.f));

	Outline->ClearInstances();
	Outline->AddInstance(BoxPiece(0.f, HalfH + T * 0.5f, BoxSize.X + T * 2.f, T));
	Outline->AddInstance(BoxPiece(0.f, -HalfH - T * 0.5f, BoxSize.X + T * 2.f, T));
	Outline->AddInstance(BoxPiece(-HalfW - T * 0.5f, 0.f, T, BoxSize.Y));
	Outline->AddInstance(BoxPiece(HalfW + T * 0.5f, 0.f, T, BoxSize.Y));
	for (int32 i = 0; i < NumTailDots; ++i)
	{
		Outline->AddInstance(BoxPiece(0.f, 0.f, T * 1.5f, T * 1.5f)); // placed by FollowPaddle
	}
}

void UIJPSpeechBubbleComponent::FollowPaddle()
{
	const AIJPPaddle* Paddle = GetPaddle();
	const AIJPArena* Arena = Paddle ? Paddle->GetArena() : nullptr;
	if (!Arena)
	{
		return;
	}

	// Beside the paddle on the net side, level with it, but never through a wall.
	const float TowardNet = -IJP::SideSign(Paddle->GetSide());
	const float HalfW = BoxSize.X * 0.5f;
	const float HalfH = BoxSize.Y * 0.5f;
	const float PaddleY = Paddle->GetPlanePosition().Y;
	const float MaxY = FMath::Max(0.f, Arena->GetHalfExtents().Y - HalfH - OutlineThickness);
	const float BoxY = FMath::Clamp(PaddleY, -MaxY, MaxY);
	SetRelativeLocation(FVector(TowardNet * (Paddle->GetSize().X * 0.5f + Gap + HalfW), 0.f, BoxY - PaddleY));

	// The tail dots step from the box's paddle-side edge toward the paddle, at the paddle's height.
	const float TailZ = FMath::Clamp(PaddleY - BoxY, -HalfH + OutlineThickness, HalfH - OutlineThickness);
	for (int32 i = 0; i < NumTailDots; ++i)
	{
		const float X = -TowardNet * (HalfW + OutlineThickness + Gap * (i + 1) / (NumTailDots + 1));
		Outline->UpdateInstanceTransform(NumEdges + i, BoxPiece(X, TailZ, OutlineThickness * 1.5f, OutlineThickness * 1.5f), false, i == NumTailDots - 1);
	}
}

void UIJPSpeechBubbleComponent::SetShownChars(int32 Chars)
{
	ShownChars = Chars;
	Text->SetText(FText::FromString(GetShownText()));
}

void UIJPSpeechBubbleComponent::Babble(TCHAR Letter) const
{
	const AIJPPaddle* Paddle = GetPaddle();
	AIJPArena* Arena = Paddle ? Paddle->GetArena() : nullptr;
	if (!Arena)
	{
		return;
	}

	// Each letter always gets the same pitch, so a word always "sounds" the same; vowels sit a
	// little higher, like stressed syllables. The speaker's voice scales the whole thing.
	const UIJPToneSet& ToneSet = Arena->GetToneSet();
	const TCHAR Lower = FChar::ToLower(Letter);
	const bool bVowel = FCString::Strchr(TEXT("aeiou"), Lower) != nullptr;
	const float Step = static_cast<float>((static_cast<int32>(Lower) * 7) % 9) / 8.f; // 0..1, scrambled by letter
	const float Semitones = FMath::Lerp(-ToneSet.TalkRange, ToneSet.TalkRange, Step) + (bVowel ? 2.f : 0.f);
	FIJPTone Tone = ToneSet.Talk;
	Tone.Frequency *= VoicePitch * FMath::Pow(2.f, Semitones / 12.f);
	Arena->GetTones()->PlayTone(Tone);
}

void UIJPSpeechBubbleComponent::Finish()
{
	Hide();
	OnLineFinished.Broadcast();
}
