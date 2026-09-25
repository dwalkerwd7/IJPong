// It's Just Pong

#include "Abilities/IJPAbility_Mirror.h"
#include "Abilities/IJPAbilityComponent.h"
#include "Gameplay/IJPArena.h"
#include "Gameplay/IJPBall.h"
#include "Gameplay/IJPPaddle.h"

void UIJPAbility_Mirror::Activate()
{
	if (!LastSeen)
	{
		return;
	}
	if (Mirrored)
	{
		Mirrored->Deactivate();
	}
	// Our own copy, run from this paddle, as if it were equipped here.
	Mirrored = DuplicateObject<UIJPAbility>(LastSeen, this);
	Mirrored->Init(GetOwnerComponent(), LastSeen);
	Mirrored->Activate();
}

bool UIJPAbility_Mirror::IsActive() const
{
	return Mirrored && Mirrored->IsActive();
}

bool UIJPAbility_Mirror::IsArmed() const
{
	return Mirrored && Mirrored->IsArmed();
}

bool UIJPAbility_Mirror::WantsAIUse() const
{
	// Whatever it copied, it's most use with a ball on its way here.
	const AIJPPaddle* Paddle = GetPaddle();
	if (!LastSeen || !Paddle || !Paddle->GetArena())
	{
		return false;
	}
	const float TowardMe = IJP::SideSign(Paddle->GetSide());
	for (const AIJPBall* Ball : Paddle->GetArena()->GetBalls())
	{
		if (Ball->IsInPlay() && Ball->GetPlaneVelocity().X * TowardMe > 0.f)
		{
			return true;
		}
	}
	return false;
}

void UIJPAbility_Mirror::TickAbility(float DeltaSeconds)
{
	Watch();
	if (Mirrored)
	{
		Mirrored->TickAbility(DeltaSeconds);
	}
}

void UIJPAbility_Mirror::OnBallHit(AIJPBall& Ball)
{
	if (Mirrored)
	{
		Mirrored->OnBallHit(Ball);
	}
}

void UIJPAbility_Mirror::Deactivate()
{
	if (Mirrored)
	{
		Mirrored->Deactivate();
	}
}

void UIJPAbility_Mirror::Watch()
{
	const AIJPPaddle* Paddle = GetPaddle();
	const AIJPArena* Arena = Paddle ? Paddle->GetArena() : nullptr;
	const AIJPPaddle* Other = Arena ? Arena->GetPaddle(IJP::Opposite(Paddle->GetSide())) : nullptr;
	UIJPAbilityComponent* OtherAbilities = Other ? Other->GetAbilities() : nullptr;
	if (OtherAbilities == Watched.Get())
	{
		return;
	}
	if (Watched.IsValid())
	{
		Watched->OnActivated.RemoveDynamic(this, &UIJPAbility_Mirror::HandleOtherActivated);
	}
	Watched = OtherAbilities;
	if (OtherAbilities)
	{
		OtherAbilities->OnActivated.AddDynamic(this, &UIJPAbility_Mirror::HandleOtherActivated);
	}
}

void UIJPAbility_Mirror::HandleOtherActivated(EIJPAbilitySlot Slot, const UIJPAbility* Ability)
{
	// Two mirrors would only copy each other.
	if (Ability && Slot != EIJPAbilitySlot::Item && !Ability->IsA<UIJPAbility_Mirror>())
	{
		LastSeen = Ability->GetDefinition();
	}
}
