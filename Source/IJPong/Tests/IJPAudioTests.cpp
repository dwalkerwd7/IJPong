// It's Just Pong

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Audio/IJPToneSet.h"
#include "Audio/IJPToneSynthComponent.h"
#include "Core/IJPTypes.h"
#include "Gameplay/IJPArena.h"
#include "Gameplay/IJPBall.h"
#include "Gameplay/IJPPaddle.h"
#include "Tests/IJPTestWorld.h"

namespace IJPAudioTests
{
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter;
}

// Audio can't be heard headless, so this checks the right beep is requested for each ball event.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPAudioEventsTest, "IJPong.Audio.BallEventsPlayTheirTones", IJPAudioTests::Flags)
bool FIJPAudioEventsTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test;
	AIJPArena* Arena = Test.GetArena();
	AIJPBall* Ball = Arena->GetBall();
	UIJPToneSynthComponent* Tones = Arena->GetTones();
	const UIJPToneSet& Set = Arena->GetToneSet();
	UTEST_NOT_NULL("Tone synth", Tones);
	UTEST_TRUE("Tone set has three distinct beeps", !(Set.PaddleHit == Set.Bounce) && !(Set.Bounce == Set.Goal) && !(Set.PaddleHit == Set.Goal));
	UTEST_EQUAL("Silent before play", Tones->GetToneCount(), 0);

	auto RunUntilTone = [&](float MaxSeconds)
	{
		const int32 Before = Tones->GetToneCount();
		for (int32 i = 0; i < FMath::CeilToInt(MaxSeconds / FIJPTestWorld::FixedStep) && Tones->GetToneCount() == Before; ++i)
		{
			Test.Step();
		}
		return Tones->GetToneCount() == Before + 1;
	};

	// Steep serve right: the top wall comes before the right (AI) paddle.
	Ball->Serve(EIJPSide::Right, 50.f);
	UTEST_TRUE("Wall bounce beeps once", RunUntilTone(2.f));
	UTEST_TRUE("Wall bounce uses the bounce tone", Tones->GetLastTone() == Set.Bounce);

	// Flat serve at the idle, centred left paddle.
	Ball->Serve(EIJPSide::Left, 0.f);
	UTEST_TRUE("Paddle hit beeps once", RunUntilTone(2.f));
	UTEST_TRUE("Paddle hit uses the paddle tone", Tones->GetLastTone() == Set.PaddleHit);

	// Move the left paddle out of the way, then a flat serve straight into its goal.
	AIJPPaddle* Left = Arena->GetPaddle(EIJPSide::Left);
	Test.RunFor(0.5f, [Left] { Left->AddMoveInput(1.f); });
	Ball->Serve(EIJPSide::Left, 0.f);
	UTEST_TRUE("Goal beeps once", RunUntilTone(2.f));
	UTEST_TRUE("Goal uses the goal tone", Tones->GetLastTone() == Set.Goal);
	return true;
}

#endif
