// It's Just Pong

#include "Abilities/IJPAbility_Spell.h"
#include "Engine/World.h"
#include "Gameplay/IJPArena.h"
#include "Gameplay/IJPBall.h"
#include "Gameplay/IJPPaddle.h"

void UIJPAbility_Spell::Activate()
{
	AIJPPaddle* Paddle = GetPaddle();
	AIJPArena* Arena = Paddle ? Paddle->GetArena() : nullptr;
	const AIJPPaddle* Target = Arena ? Arena->GetPaddle(IJP::Opposite(Paddle->GetSide())) : nullptr;
	if (!Target)
	{
		return;
	}

	// Aimed where they are now (several spread along the lane around it): moving is the dodge.
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	const float Limit = Arena->GetHalfExtents().Y - Strike.HalfHeight;
	for (int32 i = 0; i < Strikes; ++i)
	{
		const float Y = FMath::Clamp(Target->GetPlanePosition().Y + (i - (Strikes - 1) * 0.5f) * StrikeSpacing, -Limit, Limit);
		AIJPSpellStrike* SpellStrike = Paddle->GetWorld()->SpawnActor<AIJPSpellStrike>(AIJPSpellStrike::StaticClass(), Arena->GetActorTransform(), Params);
		if (SpellStrike)
		{
			SpellStrike->Launch(Arena, Paddle->GetSide(), Target->GetSide(), Y, Strike);
			LastStrike = SpellStrike;
		}
	}
}

bool UIJPAbility_Spell::WantsAIUse() const
{
	// With a ball on its way to them, so dodging costs them the ball.
	const AIJPPaddle* Paddle = GetPaddle();
	if (!Paddle || !Paddle->GetArena())
	{
		return false;
	}
	const float TowardThem = -IJP::SideSign(Paddle->GetSide());
	for (const AIJPBall* Ball : Paddle->GetArena()->GetBalls())
	{
		if (Ball->IsInPlay() && Ball->GetPlaneVelocity().X * TowardThem > 0.f)
		{
			return true;
		}
	}
	return false;
}
