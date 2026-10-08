// It's Just Pong

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "IJPFightIntroComponent.generated.h"

class UTextRenderComponent;

/** One card of a fight intro: up to three words across the screen, held for a moment. */
struct FIJPIntroCard
{
	FString Left;
	FString Centre;
	FString Right;
	float Duration = 1.f;
};

/**
 * Big text cards over the court before a match (the Fighting game era's "YOU VS RIVAL", then
 * "ROUND 1", "FIGHT!"). Play shows them in turn, each with a stinger from the tone set, then calls
 * back. On the arena, in front of everything, in the palette's score colour.
 */
UCLASS(ClassGroup = (IJPong))
class IJPONG_API UIJPFightIntroComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	UIJPFightIntroComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Show Cards one after another, then call OnDone. Replaces anything playing (without calling its OnDone). */
	void Play(const TArray<FIJPIntroCard>& Cards, TFunction<void()> OnDone);

	/** Stop and hide, without calling back. */
	void Stop();

	bool IsPlaying() const { return CardIndex != INDEX_NONE; }

	/** The card showing now, as "LEFT|CENTRE|RIGHT" (tests). */
	FString GetShownCard() const;

	UPROPERTY(EditAnywhere, Category = "Intro", meta = (ClampMin = "1"))
	float CentreSize = 90.f;

	UPROPERTY(EditAnywhere, Category = "Intro", meta = (ClampMin = "1"))
	float SideSize = 40.f;

private:
	void ShowCard(int32 Index);
	UTextRenderComponent* MakeText(const TCHAR* Name);

	UPROPERTY(Transient)
	TObjectPtr<UTextRenderComponent> LeftText;

	UPROPERTY(Transient)
	TObjectPtr<UTextRenderComponent> CentreText;

	UPROPERTY(Transient)
	TObjectPtr<UTextRenderComponent> RightText;

	TArray<FIJPIntroCard> Playing;
	TFunction<void()> Done;
	int32 CardIndex = INDEX_NONE;
	float CardLeft = 0.f;
};
