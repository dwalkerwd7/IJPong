// It's Just Pong

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Core/IJPTypes.h"
#include "IJPConversationPlayer.generated.h"

class AIJPArena;
class UIJPConversation;
class UIJPSpeechBubbleComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FIJPConversationFinishedSignature, const UIJPConversation*, Conversation);

/**
 * Plays a conversation line by line in the paddles' chat bubbles: each line waits for the one
 * before to finish. Only one conversation at a time; starting another replaces it.
 */
UCLASS(ClassGroup = (IJPong))
class IJPONG_API UIJPConversationPlayer : public UActorComponent
{
	GENERATED_BODY()

public:
	/** Play Conversation on Arena's paddles; PlayerSide is who "Player" lines belong to. */
	UFUNCTION(BlueprintCallable, Category = "Conversation")
	void Play(const UIJPConversation* Conversation, AIJPArena* InArena, EIJPSide InPlayerSide);

	/** Cut the conversation off now: bubbles down, no OnFinished. */
	UFUNCTION(BlueprintCallable, Category = "Conversation")
	void Stop();

	UFUNCTION(BlueprintPure, Category = "Conversation")
	bool IsPlaying() const { return Current != nullptr; }

	/** Seconds between one line coming down and the next going up. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Conversation", meta = (ClampMin = "0", Units = "s"))
	float GapBetweenLines = 0.25f;

	/** The last line of a conversation finished. */
	UPROPERTY(BlueprintAssignable, Category = "Conversation")
	FIJPConversationFinishedSignature OnFinished;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UFUNCTION()
	void HandleLineFinished();

	void PlayLine(int32 Index);
	void PlayNextLine();
	void Unbind();

	UPROPERTY(Transient)
	TObjectPtr<const UIJPConversation> Current;

	TWeakObjectPtr<AIJPArena> Arena;
	TWeakObjectPtr<UIJPSpeechBubbleComponent> Speaking;
	EIJPSide SpeakingSide = EIJPSide::Left;
	FTimerHandle GapTimer;
	EIJPSide PlayerSide = EIJPSide::Left;
	int32 LineIndex = 0;
};
