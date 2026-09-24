// It's Just Pong

#include "Audio/IJPToneSynthComponent.h"

namespace
{
	// Fade in/out at each end of a beep. A hard-edged square wave starting mid-cycle clicks.
	constexpr float EdgeFadeSeconds = 0.002f;
}

UIJPToneSynthComponent::UIJPToneSynthComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	NumChannels = 1;
	// One speaker in the cabinet: the beeps don't come from anywhere in particular.
	bAllowSpatialization = false;
	// Run continuously (outputting silence between beeps), so PlayTone never waits for a voice to start.
	bAutoActivate = true;
}

bool UIJPToneSynthComponent::Init(int32& SampleRate)
{
	VoiceSampleRate = static_cast<float>(SampleRate);
	return true;
}

void UIJPToneSynthComponent::PlayTone(const FIJPTone& Tone)
{
	++ToneCount;
	LastTone = Tone;

	SynthCommand([this, Tone]
	{
		VoicePhase = 0.f;
		VoicePhaseStep = Tone.Frequency / VoiceSampleRate;
		VoiceAmplitude = Tone.Volume;
		VoiceSamplesTotal = FMath::Max(1, FMath::RoundToInt(Tone.Duration * VoiceSampleRate));
		VoiceSamplesLeft = VoiceSamplesTotal;
	});
}

int32 UIJPToneSynthComponent::OnGenerateAudio(float* OutAudio, int32 NumSamples)
{
	// Mono, so one sample per frame.
	const int32 FadeSamples = FMath::Max(1, FMath::RoundToInt(EdgeFadeSeconds * VoiceSampleRate));
	for (int32 i = 0; i < NumSamples; ++i)
	{
		if (VoiceSamplesLeft <= 0)
		{
			OutAudio[i] = 0.f;
			continue;
		}

		const int32 Elapsed = VoiceSamplesTotal - VoiceSamplesLeft;
		const float Fade = FMath::Min(1.f, FMath::Min(Elapsed, VoiceSamplesLeft) / static_cast<float>(FadeSamples));
		const float Square = VoicePhase < 0.5f ? 1.f : -1.f;
		OutAudio[i] = Square * VoiceAmplitude * Fade;

		VoicePhase += VoicePhaseStep;
		VoicePhase -= FMath::FloorToFloat(VoicePhase);
		--VoiceSamplesLeft;
	}
	return NumSamples;
}
