// It's Just Pong

#include "Abilities/IJPAbility_Reader.h"
#include "Abilities/IJPAbility_Catch.h"
#include "Abilities/IJPAbilityComponent.h"
#include "AI/IJPPaddleAIController.h"
#include "Gameplay/IJPArena.h"
#include "Gameplay/IJPBall.h"
#include "Gameplay/IJPPaddle.h"
#include "Gameplay/IJPPongMath.h"

bool UIJPAbility_Reader::WantsAIUse() const
{
	const AIJPPaddle* Target = GetTarget();
	if (!Target)
	{
		return false;
	}
	const UIJPAbility_Catch* Catch = Cast<UIJPAbility_Catch>(Target->GetAbilities()->GetAbility(EIJPAbilitySlot::ClassSkill));
	return (Catch && Catch->IsArmed()) || FindHeldBall() != nullptr;
}

void UIJPAbility_Reader::TickAbility(float DeltaSeconds)
{
	if (TimeLeft <= 0.f)
	{
		return;
	}
	TimeLeft -= DeltaSeconds;
	AIJPPaddleAIController* AI = GetPaddle() ? Cast<AIJPPaddleAIController>(GetPaddle()->GetController()) : nullptr;
	float Y = 0.f;
	if (TimeLeft > 0.f && AI && PredictHeldShot(Y))
	{
		AI->SetReadTarget(Y);
	}
	else
	{
		StopReading();
	}
}

void UIJPAbility_Reader::Deactivate()
{
	TimeLeft = 0.f;
	StopReading();
}

bool UIJPAbility_Reader::PredictHeldShot(float& OutY) const
{
	const AIJPPaddle* Paddle = GetPaddle();
	const AIJPPaddle* Target = GetTarget();
	const AIJPBall* Ball = FindHeldBall();
	if (!Paddle || !Target || !Ball)
	{
		return false;
	}

	// The shot as it would leave now: a catch's own limit on the aim, else the plain aim.
	const UIJPAbility_Catch* Catch = Cast<UIJPAbility_Catch>(Target->GetAbilities()->GetAbility(EIJPAbilitySlot::ClassSkill));
	const float Angle = Catch ? Catch->GetFireAngle() : Target->GetAimAngle();
	const FVector2D Direction = Target->AimAngleToDirection(Angle);

	const FVector2D HalfExtents = Paddle->GetArena()->GetHalfExtents();
	const float BallHalf = Ball->GetSize() * 0.5f;
	const float FaceX = Paddle->GetPlanePosition().X - IJP::SideSign(Paddle->GetSide()) * (Paddle->GetSize().X * 0.5f + BallHalf);
	const float BallLimitY = HalfExtents.Y - BallHalf;
	if (!FIJPPongMath::PredictInterceptY(Ball->GetPlanePosition(), Direction, FaceX, -BallLimitY, BallLimitY, OutY))
	{
		return false;
	}
	const float PaddleLimitY = HalfExtents.Y - Paddle->GetSize().Y * 0.5f;
	OutY = FMath::Clamp(OutY, -PaddleLimitY, PaddleLimitY);
	return true;
}

AIJPPaddle* UIJPAbility_Reader::GetTarget() const
{
	const AIJPPaddle* Paddle = GetPaddle();
	AIJPArena* Arena = Paddle ? Paddle->GetArena() : nullptr;
	return Arena ? Arena->GetPaddle(IJP::Opposite(Paddle->GetSide())) : nullptr;
}

AIJPBall* UIJPAbility_Reader::FindHeldBall() const
{
	const AIJPPaddle* Target = GetTarget();
	if (!Target)
	{
		return nullptr;
	}
	for (AIJPBall* Ball : Target->GetArena()->GetBalls())
	{
		if (Ball->GetHolder() == Target)
		{
			return Ball;
		}
	}
	return nullptr;
}

void UIJPAbility_Reader::StopReading()
{
	if (AIJPPaddleAIController* AI = GetPaddle() ? Cast<AIJPPaddleAIController>(GetPaddle()->GetController()) : nullptr)
	{
		AI->ClearReadTarget();
	}
}
