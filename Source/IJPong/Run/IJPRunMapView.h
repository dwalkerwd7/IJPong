// It's Just Pong

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Presentation/IJPBlinker.h"
#include "Run/IJPRunMap.h"
#include "IJPRunMapView.generated.h"

class AIJPArena;
class UCameraComponent;
class UIJPCRTComponent;
class UInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;
class UStaticMeshComponent;
class UTextRenderComponent;
class UTexture2D;
class UIJPSkillTree;

/** Icons for the map and the skill tree, drawn in place of their letters in eras with sprites. */
USTRUCT()
struct FIJPMapIcons
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Icons")
	TSoftObjectPtr<UTexture2D> Match;

	UPROPERTY(EditAnywhere, Category = "Icons")
	TSoftObjectPtr<UTexture2D> Elite;

	UPROPERTY(EditAnywhere, Category = "Icons")
	TSoftObjectPtr<UTexture2D> Rest;

	UPROPERTY(EditAnywhere, Category = "Icons")
	TSoftObjectPtr<UTexture2D> Shop;

	UPROPERTY(EditAnywhere, Category = "Icons")
	TSoftObjectPtr<UTexture2D> Event;

	UPROPERTY(EditAnywhere, Category = "Icons")
	TSoftObjectPtr<UTexture2D> Boss;

	/** The skill tree's root (the class skill). */
	UPROPERTY(EditAnywhere, Category = "Icons")
	TSoftObjectPtr<UTexture2D> ClassSkill;

	UPROPERTY(EditAnywhere, Category = "Icons")
	TSoftObjectPtr<UTexture2D> StatNode;

	UPROPERTY(EditAnywhere, Category = "Icons")
	TSoftObjectPtr<UTexture2D> Keystone;
};

/**
 * The run's map, drawn like everything else in the game: boxes and glyph letters on a 4:3 tube
 * with the era's palette and CRT. It reads top to bottom, from the first choice of fights down
 * to the boss. It only shows the run (UIJPRunSubsystem) and keeps the player's pick among the
 * nodes they can go to next; the run game mode decides what happens.
 */
UCLASS(Config = Game)
class IJPONG_API AIJPRunMapView : public AActor
{
	GENERATED_BODY()

public:
	AIJPRunMapView();

	/** Take the arena's look (palette, materials, screen size and camera settings). Call once after spawning. */
	void Init(AIJPArena* InArena);

	/** Redraw from the run's current state (and leave card mode). */
	void Refresh();

	/** One card in a pick: a title and a few lines under it. */
	struct FCard
	{
		FString Title;
		FString Text;
	};

	/** Show a row of cards to pick from instead of the map, under Heading. Refresh() goes back to the map. */
	void ShowCards(const FString& Heading, const TArray<FCard>& Cards, int32 InSelectedCard = 0);

	UFUNCTION(BlueprintPure, Category = "Map")
	bool IsShowingCards() const { return bShowingCards; }

	/** The picked card, 0 = leftmost. */
	int32 GetSelectedCard() const { return SelectedCard; }

	/**
	 * Show a class's skill tree (ownership and prices from UIJPMetaSubsystem) instead of the map:
	 * the class skill as the root at the top, three branches hanging below, and a START RUN box.
	 * The pick steps through the nodes branch by branch, then START. Refresh() goes back to the map.
	 */
	void ShowTree(const UIJPSkillTree* Tree, bool bResetPick);

	UFUNCTION(BlueprintPure, Category = "Map")
	bool IsShowingTree() const { return ShownTree != nullptr; }

	/** The picked tree node (index into the tree), or INDEX_NONE when START RUN is picked. */
	int32 GetSelectedTreeNode() const;

	/** GetSelectedTreeNode's value for the UNLOCK SPELLS box (shown until the Spell slot is bought). */
	static constexpr int32 TreeUnlockSpells = -2;

	/**
	 * The era change: the tube switches off (the picture collapses to a line, then a dot), then
	 * Title shows on the dark screen, all within Duration. Refresh() afterwards shows the new map.
	 */
	void PlayEraChange(const FString& Title, float Duration);

	UFUNCTION(BlueprintPure, Category = "Map")
	bool IsPlayingEraChange() const { return EraChangeLeft > 0.f; }

	/**
	 * The run's end. A loss: "IT'S JUST PONG..." types out slowly (a low beep per letter), then
	 * Details. A win: "CONGRATULATIONS! YOU WON!", a fanfare, then fireworks until the screen
	 * changes. Refresh() / ShowTree() leave it.
	 */
	void ShowRunEnd(bool bWon, const FString& Details);

	UFUNCTION(BlueprintPure, Category = "Map")
	bool IsShowingRunEnd() const { return EndMode != EEndMode::None; }

	bool IsWinScreen() const { return EndMode == EEndMode::Win; }

	/** The big line as it's shown right now (it types out on a loss). */
	FString GetEndTitle() const;

	/** Firework sparks in the air right now. */
	int32 GetSparkCount() const { return Sparks.Num(); }

	/** A bright flash that fades, like a tube warming up. */
	void WarmUp();

	virtual void Tick(float DeltaSeconds) override;

	/** Move the pick one node (or card) left (-1) or right (+1). */
	void Step(int32 Direction);

	/** The picked node (index into the run's map), or INDEX_NONE if there's nowhere to go. */
	int32 GetSelectedNode() const;

	/** The line along the bottom (controls, or how the run ended). */
	void SetFooter(const FString& Text);

	/** Replace the line along the top (until the next Refresh). */
	void SetHeader(const FString& Text);

	UCameraComponent* GetCamera() const { return Camera; }

	/** Icons showing now (sprite eras draw icons instead of letters). */
	int32 GetShownIconCount() const;

	/** Node letters showing now. */
	int32 GetShownGlyphCount() const;

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

	UPROPERTY(EditAnywhere, Category = "Map|Layout")
	float CardTextSize = 13.f;

	/** An icon's size, as a fraction of its node's frame. */
	UPROPERTY(EditAnywhere, Category = "Map|Layout", meta = (ClampMin = "0.1", ClampMax = "1"))
	float IconScale = 0.75f;

	/** The icons (DefaultGame.ini). */
	UPROPERTY(Config, EditAnywhere, Category = "Map|Look")
	FIJPMapIcons Icons;

private:
	FVector2D NodePosition(int32 Node) const;
	void AddFrame(UInstancedStaticMeshComponent* Target, const FVector2D& Centre, float Width, float Height) const;
	void AddDashes(UInstancedStaticMeshComponent* Target, const FVector2D& From, const FVector2D& To) const;
	UTextRenderComponent* MakeText(float Size);

	/** Mark a node: its icon in sprite eras (when there is one), else its letter. Index is the node's glyph slot. */
	void DrawMark(int32 Index, const TCHAR* Letter, const TSoftObjectPtr<UTexture2D>& Icon, const FVector2D& At, float BoxSize, const FLinearColor& Colour);
	const TSoftObjectPtr<UTexture2D>& IconFor(EIJPNodeType Type) const;
	void ApplyColours();
	void PlaceCursor();
	void ClearDrawing();
	FVector2D CardCentre(int32 Card) const;
	FVector2D CardSize() const;
	FVector2D TreeNodePosition(int32 Node) const;
	FVector2D TreeRootPosition() const;
	FVector2D TreeStartPosition() const;
	FVector2D TreeUnlockPosition() const;

	TWeakObjectPtr<AIJPArena> Arena;
	FVector2D HalfScreen = FVector2D(400.f, 300.f);

	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextRenderComponent>> Glyphs;

	/** Icon quads, one per glyph slot, made as needed. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> IconQuads;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> IconMaterials;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> BrightMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> MidMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> DimMaterial;

	/** Title and text components for the cards, two per card. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextRenderComponent>> CardTexts;

	TArray<int32> Reachable;
	int32 Selected = 0;
	UPROPERTY(Transient)
	TObjectPtr<const UIJPSkillTree> ShownTree;

	/** Tree nodes in pick order (branch by branch, top down), then INDEX_NONE for START RUN. */
	TArray<int32> TreeOrder;
	int32 TreeSelected = 0;

	/** The picked node's name, text and price; and START RUN's label. */
	UPROPERTY(Transient)
	TObjectPtr<UTextRenderComponent> InfoText;

	UPROPERTY(Transient)
	TObjectPtr<UTextRenderComponent> StartText;

	UPROPERTY(Transient)
	TObjectPtr<UTextRenderComponent> UnlockText;

	enum class EEndMode : uint8 { None, Loss, Win };

	struct FSpark
	{
		FVector2D Position;
		FVector2D Velocity;
		float Age = 0.f;
		float Life = 1.f;
		int32 Colour = 0;
	};

	void TickRunEnd(float DeltaSeconds);
	void Burst();
	void StopRunEnd();

	/** Firework sparks, one piece set per palette colour (left paddle, right paddle, ball). */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UInstancedStaticMeshComponent>> SparkPieces;

	UPROPERTY(Transient)
	TObjectPtr<UTextRenderComponent> EndTitleText;

	UPROPERTY(Transient)
	TObjectPtr<UTextRenderComponent> EndSubText;

	UPROPERTY(Transient)
	TObjectPtr<UTextRenderComponent> EndDetailsText;

	TArray<FSpark> Sparks;
	FString EndTitle;
	FString EndDetails;
	EEndMode EndMode = EEndMode::None;
	float EndTime = 0.f;
	int32 EndLettersShown = 0;
	int32 FanfareNote = 0;
	float NextFanfareAt = 0.f;
	float NextBurstAt = 0.f;

	/** The era change's title card. */
	UPROPERTY(Transient)
	TObjectPtr<UTextRenderComponent> EraTitleText;

	float EraChangeTime = 0.f;
	float EraChangeLeft = 0.f;

	int32 NumCards = 0;
	int32 SelectedCard = 0;
	bool bShowingCards = false;
	FIJPBlinker CursorBlinker;
};
