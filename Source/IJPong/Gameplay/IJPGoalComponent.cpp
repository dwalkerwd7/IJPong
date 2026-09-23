// It's Just Pong

#include "Gameplay/IJPGoalComponent.h"

UIJPGoalComponent::UIJPGoalComponent()
{
	IJP::ConfigureAsBallBlocker(this);
	ShapeColor = FColor::Red;
}
