// It's Just Pong

#include "Run/IJPEvent.h"
#include "Run/IJPReward.h"
#include "Run/IJPRunSubsystem.h"

void FIJPEventOutcome::Apply(UIJPRunSubsystem& Run) const
{
	Run.AddCoins(Coins);
	if (MaxHealth > 0.f)
	{
		Run.AddMaxHealth(MaxHealth);
	}
	if (Health > 0.f)
	{
		Run.RestoreHealth(Health);
	}
	else if (Health < 0.f)
	{
		// An event can hurt, but never end the run on its own.
		Run.LoseHealth(FMath::Min(-Health, Run.GetHealth() - 1.f));
	}
	if (Reward)
	{
		Reward->Grant(Run);
	}
}

FString FIJPEventOutcome::Summary() const
{
	TArray<FString> Parts;
	if (Coins != 0)
	{
		Parts.Add(FString::Printf(TEXT("%+d COINS"), Coins));
	}
	if (Health != 0.f)
	{
		Parts.Add(FString::Printf(TEXT("%+d HP"), FMath::RoundToInt(Health)));
	}
	if (MaxHealth > 0.f)
	{
		Parts.Add(FString::Printf(TEXT("+%d MAX HP"), FMath::RoundToInt(MaxHealth)));
	}
	if (Reward)
	{
		Parts.Add(Reward->DisplayName.ToString().ToUpper());
	}
	return FString::Join(Parts, TEXT("\n"));
}

bool FIJPEventOption::CanChoose(const UIJPRunSubsystem& Run) const
{
	// The coins it could cost must be in hand.
	const int32 WorstCoins = FMath::Min(Success.Coins, Chance < 1.f || bMatch ? Failure.Coins : 0);
	return Run.GetCoins() >= -FMath::Min(WorstCoins, 0);
}

FString FIJPEventOption::CardText() const
{
	if (!Description.IsEmpty())
	{
		return Description.ToString();
	}
	const FString Win = Success.Summary();
	if (bMatch)
	{
		const FString Lose = Failure.Summary();
		return FString::Printf(TEXT("A MATCH\nWIN: %s\nLOSE: %s"),
			Win.IsEmpty() ? TEXT("NOTHING") : *Win.Replace(TEXT("\n"), TEXT(" ")), Lose.IsEmpty() ? TEXT("NOTHING") : *Lose.Replace(TEXT("\n"), TEXT(" ")));
	}
	if (Chance >= 1.f)
	{
		return Win.IsEmpty() ? FString(TEXT("NOTHING")) : Win;
	}
	const FString Lose = Failure.Summary();
	return FString::Printf(TEXT("%d%%: %s\nELSE: %s"), FMath::RoundToInt(Chance * 100.f),
		Win.IsEmpty() ? TEXT("NOTHING") : *Win.Replace(TEXT("\n"), TEXT(" ")), Lose.IsEmpty() ? TEXT("NOTHING") : *Lose.Replace(TEXT("\n"), TEXT(" ")));
}
