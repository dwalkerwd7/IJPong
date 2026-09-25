// It's Just Pong

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Presentation/IJPBlinker.h"
#include "IJPRunMapView.generated.h"

class AIJPArena;
class UCameraComponent;
class UIJPCRTComponent;
class UInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;
class UStaticMeshComponent;
class UTextRenderComponent;

/**
 * The run's map, drawn like everything else in the game: boxes and glyph letters on a 4:3 tube
 * with the era's palette and CRT. It reads top to bottom, from the first choice of fights down
 * to the boss. It only shows the run (UIJPRunSubsystem) and keeps the player's pick among the
 * nodes they can go to next; the run game mode decides what happens.
 */
UCLASS()
class IJPONG_API AIJPRunMapView : public AActor
{
	GENERATED_BODY()

public:
	AIJPRunMapView();

	/** Take the arena's look (palette, materials, screen size and camera settings). Call once after spawning. */
	void Init(AIJPArena* InArena);

	/** Redraw from the run's current state. */
	void Refresh();

	/** Move the pick one reachable node left (-1) or right (+1). */
	void Step(int32 Direction);

	/** The picked node (index into the run's map), or INDEX_NONE if there's nowhere to go. */
	int32 GetSelectedNode() const;

	/** The line along the bottom (controls, or how the run ended). */
	void SetFooter(const FString& Text);

	UCameraComponent* GetCamera() const { return Camera; }

protected:
	UPROPERTY(VisibleAnywhere, Category = "Map|Components")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, Category = "Map|Components")
	TObjectPtr<UCameraComponent> Camera;

	UPROPERTY(VisibleAnywhere, Category = "Map|Components")
	TObjectPtr<UIJPCRTComponent> CRT;

	UPROPERTY(VisibleAnywhere, Category = "Map|Components")
	TObjectPtr<UStaticMeshComponent> Background;

	/** Node frames and path dashes, one component per brightness. */
	UPROPERTY(VisibleAnywhere, Category = "Map|Components")
	TObjectPtr<UInstancedStaticMeshComponent> BrightPieces;

	UPROPERTY(VisibleAnywhere, Category = "Map|Components")
	TObjectPtr<UInstancedStaticMeshComponent> MidPieces;

	UPROPERTY(VisibleAnywhere, Category = "Map|Components")
	TObjectPtr<UInstancedStaticMeshComponent> DimPieces;

	/** The blinking bracket around the picked node. */
	UPROPERTY(VisibleAnywhere, Category = "Map|Components")
	TObjectPtr<UInstancedStaticMeshComponent> Cursor;

	UPROPERTY(VisibleAnywhere, Category = "Map|Components")
	TObjectPtr<UTextRenderComponent> Header;

	UPROPERTY(VisibleAnywhere, Category = "Map|Components")
	TObjectPtr<UTextRenderComponent> Footer;

	UPROPERTY(EditAnywhere, Category = "Map|Layout")
	float NodeSize = 34.f;

	UPROPERTY(EditAnywhere, Category = "Map|Layout")
	float LineThickness = 2.f;

	UPROPERTY(EditAnywhere, Category = "Map|Layout")
	float GlyphSize = 22.f;

	UPROPERTY(EditAnywhere, Category = "Map|Layout")
	float TextSize = 18.f;

private:
	FVector2D NodePosition(int32 Node) const;
	void AddFrame(UInstancedStaticMeshComponent* Target, const FVector2D& Centre, float Size) const;
	void AddDashes(UInstancedStaticMeshComponent* Target, const FVector2D& From, const FVector2D& To) const;
	UTextRenderComponent* MakeText(float Size);
	void ApplyColours();
	void PlaceCursor();

	TWeakObjectPtr<AIJPArena> Arena;
	FVector2D HalfScreen = FVector2D(400.f, 300.f);

	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextRenderComponent>> Glyphs;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> BrightMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> MidMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> DimMaterial;

	TArray<int32> Reachable;
	int32 Selected = 0;
	FIJPBlinker CursorBlinker;
};
