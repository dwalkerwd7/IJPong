// It's Just Pong

#include "Core/IJPTypes.h"
#include "Components/PrimitiveComponent.h"

DEFINE_LOG_CATEGORY(LogIJPong);

namespace IJP
{
	void ConfigureAsBallBlocker(UPrimitiveComponent* Component)
	{
		check(Component);
		// Query-only: the ball sweeps against it, but it never simulates or pushes anything.
		Component->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		Component->SetCollisionObjectType(ECC_WorldStatic);
		Component->SetCollisionResponseToAllChannels(ECR_Ignore);
		Component->SetCollisionResponseToChannel(ECC_PongBall, ECR_Block);
		Component->SetGenerateOverlapEvents(false);
		Component->SetCanEverAffectNavigation(false);
	}

	void ConfigureAsVisualOnly(UPrimitiveComponent* Component)
	{
		check(Component);
		Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Component->SetCollisionResponseToAllChannels(ECR_Ignore);
		Component->SetGenerateOverlapEvents(false);
		Component->SetCanEverAffectNavigation(false);
	}
}
