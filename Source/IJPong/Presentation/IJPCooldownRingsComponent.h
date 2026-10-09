// It's Just Pong

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "Abilities/IJPAbility.h"
#include "IJPCooldownRingsComponent.generated.h"

class AIJPPaddle;
class UInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;
class UStaticMeshComponent;
class UTextRenderComponent;

/**
 * The player's ability state under their health: a ring per cooldown slot (class skill, run ability)
 * that fills clockwise as it recharges and blinks once it's ready, with the slot's key inside; and
 * under them, the item held, as a framed icon (in sprite eras, when the item has one) or its initial.
 * A slot with nothing in it shows nothing. Laid out in the arena's X / Z plane like the charge pips.
 */
UCLASS(ClassGroup = (IJPong))
class IJPONG_API UIJPCooldownRingsComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	UIJPCooldownRingsComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Follow Paddle's abilities (null hides everything). */
	void SetPaddle(AIJPPaddle* InPaddle);

	AIJPPaddle* GetPaddle() const { return Paddle.Get(); }

	/** How full the slot's ring is (1 = ready), or -1 when it isn't shown. */
	float GetFill(EIJPAbilitySlot Slot) const;

	/** The slot just came back and its ring is blinking. */
	bool IsBlinking(EIJPAbilitySlot Slot) const;

	/** The item box is up (an unused item is held). */
	bool IsItemShown() const;

	UPROPERTY(EditAnywhere, Category = "Cooldown Rings", meta = (ClampMin = "4"))
	int32 Segments = 16;

	UPROPERTY(EditAnywhere, Category = "Cooldown Rings", meta = (ClampMin = "1"))
	float Radius = 12.f;

	/** Centre to centre, between the two rings. */
	UPROPERTY(EditAnywhere, Category = "Cooldown Rings", meta = (ClampMin = "1"))
	float RingSpacing = 36.f;

	/** How long a ring blinks when its slot is ready again. */
	UPROPERTY(EditAnywhere, Category = "Cooldown Rings", meta = (ClampMin = "0", Units = "s"))
	float ReadyBlinkTime = 0.6f;

	/** The item box's side, and its gap under the rings. */
	UPROPERTY(EditAnywhere, Category = "Cooldown Rings", meta = (ClampMin = "1"))
	float ItemSize = 24.f;

	UPROPERTY(EditAnywhere, Category = "Cooldown Rings", meta = (ClampMin = "0"))
	float ItemGap = 10.f;

private:
	struct FRing
	{
		EIJPAbilitySlot Slot = EIJPAbilitySlot::ClassSkill;
		const TCHAR* Key = TEXT("");
		float Fill = -1.f;
		float BlinkLeft = 0.f;
	};

	void EnsurePieces();
	void UpdateItem();
	void Redraw();

	TWeakObjectPtr<AIJPPaddle> Paddle;
	FRing Rings[2];

	UPROPERTY(Transient)
	TObjectPtr<UInstancedStaticMeshComponent> Pieces;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextRenderComponent>> Labels;

	UPROPERTY(Transient)
	TObjectPtr<UTextRenderComponent> ItemLetter;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> ItemIcon;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> ItemIconMaterial;

	/** What's drawn, so the pieces are only rebuilt when it changes. */
	FString DrawnKey;
	TWeakObjectPtr<const UIJPAbility> ShownItem;
	bool bItemIconShown = false;
};
