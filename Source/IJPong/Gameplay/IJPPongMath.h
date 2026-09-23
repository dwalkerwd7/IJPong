// It's Just Pong

#pragma once

#include "CoreMinimal.h"

/**
 * Stateless Pong maths, shared by the ball and the AI. Everything here works in arena plane space
 * (see IJPTypes.h): X runs between the goals, Y is vertical.
 */
struct IJPONG_API FIJPPongMath
{
	/**
	 * Outgoing velocity after a paddle hit. Classic Pong: the angle depends only on where the ball hit the paddle.
	 * @param HitOffsetNorm  -1 = bottom edge, 0 = centre, +1 = top edge (clamped).
	 * @param Speed          Outgoing speed.
	 * @param MaxAngleDeg    Angle from horizontal at the paddle's edge.
	 * @param DirX           +1 to send the ball right, -1 to send it left.
	 */
	static FVector2D ComputePaddleBounce(float HitOffsetNorm, float Speed, float MaxAngleDeg, float DirX);

	/** Mirror Velocity across a surface with the given normal (normalised internally). Speed is preserved. */
	static FVector2D Reflect(const FVector2D& Velocity, const FVector2D& Normal);

	/**
	 * Keep a velocity within MaxAngleDeg of horizontal, preserving speed and the signs of X and Y.
	 * Stops wall/corner bounces from producing near-vertical balls that never reach a paddle.
	 */
	static FVector2D ClampAngle(const FVector2D& Velocity, float MaxAngleDeg);

	/**
	 * Predict the Y at which a ball will cross LaneX, accounting for any number of wall bounces.
	 * MinY/MaxY are the limits of the ball's *centre* (wall inner edge minus ball half-size).
	 * @return false if the ball is not moving toward LaneX.
	 */
	static bool PredictInterceptY(const FVector2D& Position, const FVector2D& Velocity, float LaneX, float MinY, float MaxY, float& OutY);

	/** Fold an unbounded coordinate back into [Min, Max] as if it bounced between the two limits. */
	static float FoldIntoRange(float Value, float Min, float Max);
};
