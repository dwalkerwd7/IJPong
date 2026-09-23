// It's Just Pong

#include "Gameplay/IJPPongMath.h"

FVector2D FIJPPongMath::ComputePaddleBounce(float HitOffsetNorm, float Speed, float MaxAngleDeg, float DirX)
{
	const float AngleRad = FMath::DegreesToRadians(FMath::Clamp(HitOffsetNorm, -1.f, 1.f) * MaxAngleDeg);
	const float SignX = DirX >= 0.f ? 1.f : -1.f;
	return FVector2D(SignX * FMath::Cos(AngleRad), FMath::Sin(AngleRad)) * Speed;
}

FVector2D FIJPPongMath::Reflect(const FVector2D& Velocity, const FVector2D& Normal)
{
	const FVector2D N = Normal.GetSafeNormal();
	if (N.IsZero())
	{
		return -Velocity;
	}
	return Velocity - 2.f * FVector2D::DotProduct(Velocity, N) * N;
}

FVector2D FIJPPongMath::ClampAngle(const FVector2D& Velocity, float MaxAngleDeg)
{
	const float Speed = Velocity.Size();
	if (Speed <= UE_KINDA_SMALL_NUMBER)
	{
		return Velocity;
	}

	const float MaxAngleRad = FMath::DegreesToRadians(MaxAngleDeg);
	const float AngleRad = FMath::Atan2(FMath::Abs(Velocity.Y), FMath::Abs(Velocity.X));
	if (AngleRad <= MaxAngleRad)
	{
		return Velocity;
	}

	// A perfectly vertical ball has no X sign to keep; send it right rather than leaving it stuck.
	const float SignX = Velocity.X >= 0.f ? 1.f : -1.f;
	const float SignY = Velocity.Y >= 0.f ? 1.f : -1.f;
	return FVector2D(SignX * FMath::Cos(MaxAngleRad), SignY * FMath::Sin(MaxAngleRad)) * Speed;
}

bool FIJPPongMath::PredictInterceptY(const FVector2D& Position, const FVector2D& Velocity, float LaneX, float MinY, float MaxY, float& OutY)
{
	if (FMath::IsNearlyZero(Velocity.X))
	{
		return false;
	}

	const float TimeToLane = (LaneX - Position.X) / Velocity.X;
	if (TimeToLane < 0.f)
	{
		return false;
	}

	// Walls act like mirrors: follow the straight line, then fold it back into the arena.
	OutY = FoldIntoRange(Position.Y + Velocity.Y * TimeToLane, MinY, MaxY);
	return true;
}

float FIJPPongMath::FoldIntoRange(float Value, float Min, float Max)
{
	const float Range = Max - Min;
	if (Range <= 0.f)
	{
		return Min;
	}

	// One "period" is a trip up and back down.
	const float Period = 2.f * Range;
	float Offset = FMath::Fmod(Value - Min, Period);
	if (Offset < 0.f)
	{
		Offset += Period;
	}
	return Min + (Offset <= Range ? Offset : Period - Offset);
}
