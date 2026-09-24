// It's Just Pong

#pragma once

#include "CoreMinimal.h"
#include "Components/SynthComponent.h"
#include "Audio/IJPToneSet.h"
#include "IJPToneSynthComponent.generated.h"

/**
 * A mono square-wave beeper, generated sample by sample. PlayTone can be called any time from the
 * game thread; a new tone cuts off the current one, like the single-voice original.
 *
 * Threading: OnGenerateAudio runs on the audio render thread. Everything it reads (the Voice* members)
 * is only ever written there too, via SynthCommand, which queues a lambda onto that thread.
 */
UCLASS(ClassGroup = (IJPong), meta = (BlueprintSpawnableComponent))
class IJPONG_API UIJPToneSynthComponent : public USynthComponent
{
	GENERATED_BODY()

public:
	UIJPToneSynthComponent(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable, Category = "Tone")
	void PlayTone(const FIJPTone& Tone);

	/** Game-thread record of requests, for tests and debugging (audio itself can't be observed headless). */
	int32 GetToneCount() const { return ToneCount; }
	const FIJPTone& GetLastTone() const { return LastTone; }

protected:
	virtual bool Init(int32& SampleRate) override;
	virtual int32 OnGenerateAudio(float* OutAudio, int32 NumSamples) override;

private:
	// Game thread.
	int32 ToneCount = 0;
	FIJPTone LastTone;

	// Audio render thread only.
	float VoiceSampleRate = 48000.f;
	float VoicePhase = 0.f;          // 0..1 through the current cycle
	float VoicePhaseStep = 0.f;      // cycles per sample
	float VoiceAmplitude = 0.f;
	int32 VoiceSamplesTotal = 0;
	int32 VoiceSamplesLeft = 0;
};
