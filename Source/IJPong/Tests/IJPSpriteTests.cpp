// It's Just Pong

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Core/IJPTypes.h"
#include "Engine/Texture2D.h"
#include "Era/IJPEra.h"
#include "Era/IJPEraSubsystem.h"
#include "Gameplay/IJPArena.h"
#include "Gameplay/IJPBall.h"
#include "Gameplay/IJPBallType.h"
#include "Gameplay/IJPPaddle.h"
#include "Gameplay/IJPPaddleClass.h"
#include "Gameplay/IJPPaddleProfile.h"
#include "Tests/IJPTestWorld.h"

namespace IJPSpriteTests
{
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter;

	UIJPEra* MakeSpriteEra(bool bClassicBalls)
	{
		UIJPEra* Era = NewObject<UIJPEra>(GetTransientPackage());
		Era->bShowSprites = true;
		Era->bClassicBallSprites = bClassicBalls;
		return Era;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPPaddleSpriteTest, "IJPong.Sprites.PaddlesWearTheirClassSpriteInSpriteEras", IJPSpriteTests::Flags)
bool FIJPPaddleSpriteTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test;
	AIJPArena* Arena = Test.GetArena();
	AIJPPaddle* Paddle = Arena->GetPaddle(EIJPSide::Left);
	UIJPEraSubsystem* Eras = UIJPEraSubsystem::Get(Arena);

	UIJPPaddleClass* PaddleClass = NewObject<UIJPPaddleClass>(GetTransientPackage());
	PaddleClass->Profile = NewObject<UIJPPaddleProfile>(GetTransientPackage());
	PaddleClass->Sprite = UTexture2D::CreateTransient(8, 32);
	Paddle->SetPaddleClass(PaddleClass);
	UTEST_FALSE("1972: a plain rectangle", Paddle->IsSpriteShown());

	Eras->SetEra(IJPSpriteTests::MakeSpriteEra(false));
	UTEST_TRUE("Sprite era: the class's sprite", Paddle->IsSpriteShown());
	UTEST_TRUE("Still drawn", Paddle->IsVisualShown());

	// A split paddle is two boxes; the sprite comes back when it closes.
	Paddle->SetSplitGap(40.f);
	UTEST_FALSE("Split: boxes", Paddle->IsSpriteShown());
	Paddle->SetSplitGap(0.f);
	UTEST_TRUE("Whole again", Paddle->IsSpriteShown());

	PaddleClass->Sprite = nullptr;
	Paddle->SetPaddleClass(PaddleClass);
	UTEST_FALSE("No sprite for the class: a rectangle", Paddle->IsSpriteShown());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIJPBallSpriteTest, "IJPong.Sprites.BallsWearTheirTypeSpriteInSpriteEras", IJPSpriteTests::Flags)
bool FIJPBallSpriteTest::RunTest(const FString& Parameters)
{
	FIJPTestWorld Test;
	AIJPArena* Arena = Test.GetArena();
	UIJPEraSubsystem* Eras = UIJPEraSubsystem::Get(Arena);

	UIJPBallType* Type = NewObject<UIJPBallType>(GetTransientPackage());
	Type->Sprite = UTexture2D::CreateTransient(16, 16);
	AIJPBall* Ball = Arena->AddBall(Type);
	UTEST_FALSE("1972: a plain square", Ball->IsSpriteShown());

	Eras->SetEra(IJPSpriteTests::MakeSpriteEra(true));
	UTEST_TRUE("Sprite era: the type's sprite", Ball->IsSpriteShown());

	UIJPBallType* Plain = NewObject<UIJPBallType>(GetTransientPackage());
	AIJPBall* PlainBall = Arena->AddBall(Plain);
	UTEST_FALSE("No sprite for the type: a square", PlainBall->IsSpriteShown());
	return true;
}

#endif
