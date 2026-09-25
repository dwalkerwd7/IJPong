// It's Just Pong

#include "Abilities/IJPAbility_Warp.h"
#include "Gameplay/IJPArena.h"
#include "Gameplay/IJPBall.h"
#include "Gameplay/IJPPaddle.h"

void UIJPAbility_Warp::Activate()
{
	AIJPBall* Ball = PickBall();
	if (!Ball)
	{
		return;
	}

	const FVector2D Position = Ball->GetPlanePosition();
	float NewY = -Position.Y;
	if (FMath::Abs(NewY - Position.Y) < MinJump)
	{
		NewY = Position.Y >= 0.f ? Position.Y - MinJump : Position.Y + MinJump;
	}
	const float MaxY = FMath::Max(GetPaddle()->GetArena()->GetHalfExtents().Y - Ball->GetSize() * 0.5f, 0.f);
	Ball->Warp(FVector2D(Position.X, FMath::Clamp(NewY, -MaxY, MaxY)));
}

bool UIJPAbility_Warp::WantsAIUse() const
{
	// Just over the net and on its way to them: the warp lands mid-way across their half.
	const AIJPPaddle* Target = GetTarget();
	if (!Target)
	{
		return false;
	}
	const float TowardThem = IJP::SideSign(Target->GetSide());
	for (const AIJPBall* Ball : Target->GetArena()->GetBalls())
	{
		const float PastNet = Ball->GetPlanePosition().X * TowardThem;
		if (Ball->IsInPlay() && Ball->GetPlaneVelocity().X * TowardThem > 0.f && PastNet >= AIPastNet && PastNet <= AIPastNet * 2.f)
		{
			return true;
		}
	}
	return false;
}

AIJPPaddle* UIJPAbility_Warp::GetTarget() const
{
	const AIJPPaddle* Paddle = GetPaddle();
	AIJPArena* Arena = Paddle ? Paddle->GetArena() : nullptr;
	return Arena ? Arena->GetPaddle(IJP::Opposite(Paddle->GetSide())) : nullptr;
}

AIJPBall* UIJPAbility_Warp::PickBall() const
{
	const AIJPPaddle* Target = GetTarget();
	if (!Target)
	{
		return nullptr;
	}
	const float TowardThem = IJP::SideSign(Target->GetSide());
	AIJPBall* Furthest = nullptr;
	for (AIJPBall* Ball : Target->GetArena()->GetBalls())
	{
		if (Ball->IsInPlay() && Ball->GetPlaneVelocity().X * TowardThem > 0.f
			&& (!Furthest || Ball->GetPlanePosition().X * TowardThem < Furthest->GetPlanePosition().X * TowardThem))
		{
			Furthest = Ball;
		}
	}
	return Furthest;
}
