// It's Just Pong

#include "Core/IJPTestHUD.h"
#include "CanvasItem.h"
#include "Core/IJPTestGameMode.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "Engine/World.h"

void AIJPTestHUD::DrawHUD()
{
	Super::DrawHUD();

	const AIJPTestGameMode* GameMode = GetWorld()->GetAuthGameMode<AIJPTestGameMode>();
	if (!bShowOverlay || !GameMode || !Canvas)
	{
		return;
	}

	TArray<FString> Lines;
	GameMode->GetDebugLines(Lines);

	UFont* Font = GEngine->GetSmallFont();
	const float LineHeight = Font->GetMaxCharHeight() + 2.f;
	FVector2D Position = Margin;
	for (const FString& Line : Lines)
	{
		// Shadowed so it stays readable over the white paddles and net.
		FCanvasTextItem Item(Position, FText::FromString(Line), Font, FLinearColor::White);
		Item.EnableShadow(FLinearColor::Black);
		Canvas->DrawItem(Item);
		Position.Y += LineHeight;
	}
}
