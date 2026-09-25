// It's Just Pong

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "IJPSpeechBubbleComponent.generated.h"

class AIJPPaddle;
class UInstancedStaticMeshComponent;
class UStaticMesh;
class UStaticMeshComponent;
class UTextRenderComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FIJPLineFinishedSignature);

/**
 * A paddle's chat bubble: a box beside the paddle, on the side facing the net, that types a line
 * out letter by letter (with a soft blip), holds it, then disappears. It's drawn in the arena
 * like everything else, so the CRT and the era palette apply, and it sits behind the paddles and
 * balls so it can never hide play. It follows the paddle and stays between the walls.
 */
UCLASS(ClassGroup = (IJPong), meta = (BlueprintSpawnableComponent))
class IJPONG_API UIJPSpeechBubbleComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	UIJPSpeechBubbleComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Show Text, replacing any line in progress. HoldTime 0 = a readable time from its length. */
	UFUNCTION(BlueprintCallable, Category = "Speech Bubble")
	void Say(const FText& Text, float HoldTime = 0.f);

	/** Take the bubble down now, without OnLineFinished. */
	UFUNCTION(BlueprintCallable, Category = "Speech Bubble")
	void Hide();

	UFUNCTION(BlueprintPure, Category = "Speech Bubble")
	bool IsTalking() const { return bTalking; }

	/** The whole line being said. */
	UFUNCTION(BlueprintPure, Category = "Speech Bubble")
	FString GetLine() const { return FullText; }

	/** The part typed out so far. */
	UFUNCTION(BlueprintPure, Category = "Speech Bubble")
	FString GetShownText() const { return FullText.Left(ShownChars); }

	/** The line was typed out, held, and taken down. */
	UPROPERTY(BlueprintAssignable, Category = "Speech Bubble")
	FIJPLineFinishedSignature OnLineFinished;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Speech Bubble", meta = (ClampMin = "1"))
	float CharsPerSecond = 30.f;

	/** Hold time for an automatic line: this, plus HoldPerChar for each letter. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Speech Bubble", meta = (ClampMin = "0", Units = "s"))
	float BaseHoldTime = 1.2f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Speech Bubble", meta = (ClampMin = "0", Units = "s"))
	float HoldPerChar = 0.05f;

	/** Height of the letters, in arena units. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Speech Bubble", meta = (ClampMin = "1"))
	float TextSize = 18.f;

	/** Space between the text and the box's edge. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Speech Bubble", meta = (ClampMin = "0"))
	float Padding = 6.f;

	/** Space between the paddle and the box. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Speech Bubble", meta = (ClampMin = "0"))
	float Gap = 14.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Speech Bubble", meta = (ClampMin = "0.5"))
	float OutlineThickness = 2.f;

private:
	AIJPPaddle* GetPaddle() const;
	void EnsurePieces();
	void ApplyLook();
	void BuildBox();
	void FollowPaddle();
	void SetShownChars(int32 Chars);
	void Blip() const;
	void Finish();

	UPROPERTY(Transient)
	TObjectPtr<UTextRenderComponent> Text;

	/** Background-coloured box that the text sits on. */
	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> Fill;

	/** The box's four edges plus two "tail" dots pointing at the paddle. */
	UPROPERTY(Transient)
	TObjectPtr<UInstancedStaticMeshComponent> Outline;

	UPROPERTY()
	TObjectPtr<UStaticMesh> CubeMesh;

	FString FullText;
	FVector2D BoxSize = FVector2D::ZeroVector;
	int32 ShownChars = 0;
	float RevealTime = 0.f;
	float HoldLeft = 0.f;
	bool bTalking = false;
};
