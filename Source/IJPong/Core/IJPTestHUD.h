// It's Just Pong

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "IJPTestHUD.generated.h"

/**
 * The test mode's debug overlay: plain text in the top-left corner, toggled with the period key (F1 is the engine's wireframe view-mode shortcut).
 * It only draws; the lines come from AIJPTestGameMode::GetDebugLines, so they can be tested headless.
 * Drawn after post-processing, so the CRT look doesn't distort it.
 */
UCLASS()
class IJPONG_API AIJPTestHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

	UFUNCTION(BlueprintCallable, Category = "Test")
	void ToggleOverlay() { bShowOverlay = !bShowOverlay; }

	UFUNCTION(BlueprintPure, Category = "Test")
	bool IsOverlayShown() const { return bShowOverlay; }

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Test")
	bool bShowOverlay = true;

	/** Distance from the top-left corner of the viewport, in pixels. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Test")
	FVector2D Margin = FVector2D(16.f, 16.f);
};
