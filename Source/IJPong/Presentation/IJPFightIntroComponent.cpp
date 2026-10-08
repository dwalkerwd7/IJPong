// It's Just Pong

#include "Presentation/IJPFightIntroComponent.h"
#include "Audio/IJPToneSet.h"
#include "Audio/IJPToneSynthComponent.h"
#include "Components/TextRenderComponent.h"
#include "Gameplay/IJPArena.h"

namespace
{
	constexpr float IntroDepth = 20.f; // in front of every piece
}

UIJPFightIntroComponent::UIJPFightIntroComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

UTextRenderComponent* UIJPFightIntroComponent::MakeText(const TCHAR* Name)
{
	UTextRenderComponent* Text = NewObject<UTextRenderComponent>(GetOwner(), Name);
	Text->SetupAttachment(this);
	Text->SetHorizontalAlignment(EHTA_Center);
	Text->SetVerticalAlignment(EVRTA_TextCenter);
	// The arena's plane faces the camera along +Y: turn the text to face it the same way.
	Text->SetRelativeRotation(FRotator(0.f, 90.f, 0.f));
	Text->SetVisibility(false);
	Text->RegisterComponent();
	return Text;
}

void UIJPFightIntroComponent::Play(const TArray<FIJPIntroCard>& Cards, TFunction<void()> OnDone)
{
	if (!LeftText)
	{
		LeftText = MakeText(TEXT("IntroLeft"));
		CentreText = MakeText(TEXT("IntroCentre"));
		RightText = MakeText(TEXT("IntroRight"));
	}
	Playing = Cards;
	Done = MoveTemp(OnDone);
	if (Playing.IsEmpty())
	{
		Stop();
		if (Done)
		{
			TFunction<void()> Callback = MoveTemp(Done);
			Callback();
		}
		return;
	}
	ShowCard(0);
	SetComponentTickEnabled(true);
}

void UIJPFightIntroComponent::Stop()
{
	CardIndex = INDEX_NONE;
	Playing.Reset();
	Done = nullptr;
	for (UTextRenderComponent* Text : { LeftText.Get(), CentreText.Get(), RightText.Get() })
	{
		if (Text)
		{
			Text->SetVisibility(false);
		}
	}
	SetComponentTickEnabled(false);
}

FString UIJPFightIntroComponent::GetShownCard() const
{
	return Playing.IsValidIndex(CardIndex) ? FString::Printf(TEXT("%s|%s|%s"), *Playing[CardIndex].Left, *Playing[CardIndex].Centre, *Playing[CardIndex].Right) : FString();
}

void UIJPFightIntroComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (CardIndex == INDEX_NONE)
	{
		return;
	}
	CardLeft -= DeltaTime;
	if (CardLeft > 0.f)
	{
		return;
	}
	if (Playing.IsValidIndex(CardIndex + 1))
	{
		ShowCard(CardIndex + 1);
		return;
	}
	TFunction<void()> Callback = MoveTemp(Done);
	Stop();
	if (Callback)
	{
		Callback();
	}
}

void UIJPFightIntroComponent::ShowCard(int32 Index)
{
	CardIndex = Index;
	const FIJPIntroCard& Card = Playing[Index];
	CardLeft = Card.Duration;
	const AIJPArena* Arena = Cast<AIJPArena>(GetOwner());
	const FColor Ink = Arena ? Arena->GetPalette().Score.ToFColor(true) : FColor::White;
	const float SideX = Arena ? Arena->GetHalfExtents().X * 0.55f : 220.f;
	struct FPiece { UTextRenderComponent* Text; const FString* Words; float X; float Size; };
	for (const FPiece& Piece : { FPiece{ LeftText, &Card.Left, -SideX, SideSize }, FPiece{ CentreText, &Card.Centre, 0.f, CentreSize }, FPiece{ RightText, &Card.Right, SideX, SideSize } })
	{
		Piece.Text->SetText(FText::FromString(*Piece.Words));
		Piece.Text->SetWorldSize(Piece.Size);
		Piece.Text->SetTextRenderColor(Ink);
		if (Arena)
		{
			Piece.Text->SetTextMaterial(Arena->GetTextMaterial());
		}
		Piece.Text->SetRelativeLocation(FVector(Piece.X, IntroDepth, 0.f));
		Piece.Text->SetVisibility(!Piece.Words->IsEmpty());
	}
	if (Arena)
	{
		Arena->GetTones()->PlayTone(Index + 1 < Playing.Num() ? Arena->GetToneSet().Arm : Arena->GetToneSet().Pop);
	}
}
