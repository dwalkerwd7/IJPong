// It's Just Pong

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "IJPSpeechBubbleComponent.generated.h"

class AIJPPaddle;
class UMaterialInstanceDynamic;
class UStaticMesh;
class UStaticMeshComponent;
class UTextRenderComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FIJPLineFinishedSignature);

/**
 * A paddle's chat bubble: a box beside the paddle, on the side facing the net, with a tail pointing
 * at the paddle, that types a line out letter by letter (babbling), holds it, then disappears.
 * Its corners and tail follow the era (square with a stepped pixel tail in 1972, rounded with a
 * smooth tail later); the panel is drawn by the M_PongBubble shader from a few parameters. It's drawn in the arena
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

	/** The speaker's voice: a multiple of the tone set's Talk pitch (1 = as set, 0.8 = lower). */
	UFUNCTION(BlueprintCallable, Category = "Speech Bubble")
	void SetVoicePitch(float Scale) { VoicePitch = FMath::Max(Scale, 0.1f); }

	UFUNCTION(BlueprintPure, Category = "Speech Bubble")
	float GetVoicePitch() const { return VoicePitch; }

	/** The current line's corner radius (0 = square), from the era when it was said. */
	UFUNCTION(BlueprintPure, Category = "Speech Bubble")
	float GetCornerRadius() const { return CornerRadius; }

	/** The current line's tail is a pixel staircase (true) or a smooth wedge (false). */
	UFUNCTION(BlueprintPure, Category = "Speech Bubble")
	bool IsTailStepped() const { return bTailStepped; }

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
	float TextSize = 22.f;

	/** Space between the text and the box's edge. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Speech Bubble", meta = (ClampMin = "0"))
	float Padding = 6.f;

	/** Space between the paddle and the box. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Speech Bubble", meta = (ClampMin = "0"))
	float Gap = 14.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Speech Bubble", meta = (ClampMin = "0.5"))
	float OutlineThickness = 2.f;

	/** Width of the tail where it leaves the box. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Speech Bubble", meta = (ClampMin = "1"))
	float TailWidth = 14.f;

	/** Size of each step of a stepped (pixel) tail. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Speech Bubble", meta = (ClampMin = "1"))
	float TailStep = 3.f;

private:
	AIJPPaddle* GetPaddle() const;
	void EnsurePieces();
	void ApplyLook();
	void BuildBox();
	void FollowPaddle();
	void SetShownChars(int32 Chars);
	/** One syllable of babble for Letter. */
	void Babble(TCHAR Letter) const;
	void Finish();

	UPROPERTY(Transient)
	TObjectPtr<UTextRenderComponent> Text;

	/** The box and its tail, in one panel drawn by the bubble material. */
	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> Panel;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> PanelMaterial;

	UPROPERTY()
	TObjectPtr<UStaticMesh> CubeMesh;

	FString FullText;
	FVector2D BoxSize = FVector2D::ZeroVector;
	int32 ShownChars = 0;
	float RevealTime = 0.f;
	float HoldLeft = 0.f;
	float VoicePitch = 1.f;
	float CornerRadius = 0.f;
	bool bTailStepped = true;
	bool bTalking = false;
};
