// It's Just Pong

#include "Abilities/IJPAbility_Curve.h"
#include "Gameplay/IJPBall.h"
#include "Gameplay/IJPPaddle.h"

void UIJPAbility_Curve::OnBallHit(AIJPBall& Ball)
{
	const AIJPPaddle* Paddle = GetPaddle();
	if (!bArmed || !Paddle)
	{
		return;
	}
	bArmed = false;

	// Bend the way the paddle was moving, like spin. Standing still, bend against the ball's own
	// slope (or upward for a flat shot), so it still visibly curves.
	const float PaddleVelocity = Paddle->GetPlaneVelocity();
	const float BallSlope = Ball.GetPlaneVelocity().Y;
	const float BendUp = FMath::Abs(PaddleVelocity) > 1.f ? FMath::Sign(PaddleVelocity)
		: FMath::Abs(BallSlope) > 1.f ? -FMath::Sign(BallSlope) : 1.f;
	Ball.Curve(DegreesPerSecond, Duration, BendUp);
}
