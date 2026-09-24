// It's Just Pong

#pragma once

#include "CoreMinimal.h"
#include "Engine/TimerHandle.h"

/**
 * Toggles something on and off on a timer: the shared engine behind every blink/flash/flicker.
 * Embed one in the object that owns the visual; it's tied to that object's lifetime.
 *
 *   Blinker.Start(this, 0.1f, 6, false, [this](bool bShow) { SetVisibility(bShow); });
 */
struct IJPONG_API FIJPBlinker
{
	/**
	 * @param Owner          Provides the world's timer manager; the timer dies with it.
	 * @param Period         Seconds between toggles.
	 * @param NumToggles     Toggles before stopping on its own; 0 = until Stop()/Cancel().
	 * @param bStartVisible  State applied immediately on start.
	 * @param InApply        Shows (true) or hides (false) the visual.
	 */
	void Start(UObject* Owner, float Period, int32 NumToggles, bool bStartVisible, TFunction<void(bool)> InApply);

	/** Stop and leave the visual shown. */
	void Stop();

	/** Stop without touching the visual (e.g. when the caller is about to hide it anyway). */
	void Cancel();

	bool IsRunning() const { return bRunning; }

private:
	void Toggle();

	TWeakObjectPtr<UObject> OwnerObject;
	FTimerHandle Timer;
	TFunction<void(bool)> Apply;
	int32 TogglesLeft = 0;
	bool bVisible = true;
	bool bRunning = false;
};
