// It's Just Pong

#include "Presentation/IJPBlinker.h"
#include "Engine/World.h"
#include "TimerManager.h"

void FIJPBlinker::Start(UObject* Owner, float Period, int32 NumToggles, bool bStartVisible, TFunction<void(bool)> InApply)
{
	Cancel();

	UWorld* World = Owner ? Owner->GetWorld() : nullptr;
	if (!World)
	{
		return;
	}

	OwnerObject = Owner;
	Apply = MoveTemp(InApply);
	TogglesLeft = NumToggles;
	bVisible = bStartVisible;
	bRunning = true;
	Apply(bVisible);

	// A weak lambda: if the owner is destroyed mid-blink, the timer just stops firing.
	World->GetTimerManager().SetTimer(Timer, FTimerDelegate::CreateWeakLambda(Owner, [this] { Toggle(); }),
		FMath::Max(Period, UE_KINDA_SMALL_NUMBER), true);
}

void FIJPBlinker::Toggle()
{
	bVisible = !bVisible;
	Apply(bVisible);

	if (TogglesLeft > 0 && --TogglesLeft == 0)
	{
		Stop();
	}
}

void FIJPBlinker::Stop()
{
	const bool bWasRunning = bRunning;
	Cancel();
	if (bWasRunning && !bVisible && Apply)
	{
		bVisible = true;
		Apply(true);
	}
}

void FIJPBlinker::Cancel()
{
	if (UObject* Owner = OwnerObject.Get())
	{
		if (UWorld* World = Owner->GetWorld())
		{
			World->GetTimerManager().ClearTimer(Timer);
		}
	}
	bRunning = false;
}
