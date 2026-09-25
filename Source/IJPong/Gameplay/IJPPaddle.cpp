// It's Just Pong

#include "Gameplay/IJPPaddle.h"
#include "Abilities/IJPAbilityComponent.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Gameplay/IJPArena.h"
#include "Gameplay/IJPPaddleClass.h"
#include "Gameplay/IJPPaddleProfile.h"
#include "UObject/ConstructorHelpers.h"

AIJPPaddle::AIJPPaddle()
{
	PrimaryActorTick.bCanEverTick = true;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));

	Collision = CreateDefaultSubobject<UBoxComponent>(TEXT("Collision"));
	SetRootComponent(Collision);
	IJP::ConfigureAsBallBlocker(Collision);

	Visual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Visual"));
	Visual->SetupAttachment(Collision);
	Visual->SetStaticMesh(CubeMesh.Object);
	Visual->CastShadow = false;
	IJP::ConfigureAsVisualOnly(Visual);

	Abilities = CreateDefaultSubobject<UIJPAbilityComponent>(TEXT("Abilities"));

	// The arena camera is the view; the paddle never owns one.
	bFindCameraComponentWhenViewTarget = false;
	// Orientation always comes from the arena, never from a controller's rotation.
	bUseControllerRotationYaw = false;
}

void AIJPPaddle::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyLayout();
}

void AIJPPaddle::InitPaddle(AIJPArena* InArena, EIJPSide InSide, float InLaneX, const UIJPPaddleClass* InClass)
{
	check(InArena);
	Arena = InArena;
	Side = InSide;
	LaneX = InLaneX;
	PlaneY = 0.f;
	Velocity = 0.f;
	PendingInput = 0.f;

	Visual->SetMaterial(0, InArena->GetPaletteMaterial(InSide == EIJPSide::Left ? EIJPPaletteRole::LeftPaddle : EIJPPaletteRole::RightPaddle));
	SetPaddleClass(InClass);
}

void AIJPPaddle::SetPaddleClass(const UIJPPaddleClass* InClass)
{
	PaddleClass = InClass;
	ApplyLayout();
	if (Arena.IsValid())
	{
		// A longer class can't stick through a wall it was resting against.
		ClampToWalls();
		UpdateTransform();
	}
	Abilities->Equip(EIJPAbilitySlot::ClassSkill, InClass ? InClass->ClassSkill.Get() : nullptr);
}

const UIJPPaddleProfile* AIJPPaddle::GetProfile() const
{
	const UIJPPaddleProfile* Profile = PaddleClass ? PaddleClass->Profile.Get() : nullptr;
	return Profile ? Profile : GetDefault<UIJPPaddleProfile>();
}

FVector2D AIJPPaddle::GetSize() const
{
	const FVector2D Size = GetProfile()->Size;
	return FVector2D(Size.X, Size.Y * LengthScale);
}

void AIJPPaddle::Dash(float Distance, float Duration)
{
	// Steering this very frame wins over the last direction (input arrives before the paddle ticks).
	const float Direction = PendingInput != 0.f ? FMath::Sign(PendingInput) : LastMoveSign;
	DashTimeLeft = FMath::Max(Duration, UE_KINDA_SMALL_NUMBER);
	DashVelocity = Direction * Distance / DashTimeLeft;
}

void AIJPPaddle::SetLengthScale(float Scale)
{
	LengthScale = FMath::Max(Scale, KINDA_SMALL_NUMBER);
	ApplyLayout();
	if (Arena.IsValid())
	{
		// Growing next to a wall pushes the paddle away from it rather than into it.
		ClampToWalls();
		UpdateTransform();
	}
}

bool AIJPPaddle::ClampToWalls()
{
	const float MaxY = FMath::Max(0.f, Arena->GetHalfExtents().Y - GetSize().Y * 0.5f);
	const float Clamped = FMath::Clamp(PlaneY, -MaxY, MaxY);
	const bool bMoved = Clamped != PlaneY;
	PlaneY = Clamped;
	return bMoved;
}

float AIJPPaddle::GetMaxSpeed() const
{
	return GetProfile()->MaxSpeed;
}

float AIJPPaddle::GetRampTime() const
{
	return GetProfile()->RampTime;
}

void AIJPPaddle::ApplyLayout()
{
	// Same local axes as the arena: X = plane X, Z = plane Y, Y = depth.
	const FVector2D Size = GetSize();
	Collision->SetBoxExtent(FVector(Size.X * 0.5f, BlockerDepth * 0.5f, Size.Y * 0.5f));

	const float CubeSize = 100.f; // /Engine/BasicShapes/Cube is 100 units, centred.
	Visual->SetRelativeScale3D(FVector(Size.X / CubeSize, VisualDepth / CubeSize, Size.Y / CubeSize));
}

void AIJPPaddle::Flicker()
{
	// Hidden now, shown again after one toggle. Only the visual: the collision never flickers.
	FlickerBlinker.Start(this, GetProfile()->FlickerTime, 1, false, [this](bool bShow) { Visual->SetVisibility(bShow); });
}

bool AIJPPaddle::IsVisualShown() const
{
	return Visual->IsVisible();
}

void AIJPPaddle::AddMoveInput(float Value)
{
	PendingInput += Value;
}

void AIJPPaddle::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Consume this frame's input, like APawn's movement input vector.
	const float Input = FMath::Clamp(PendingInput, -1.f, 1.f);
	PendingInput = 0.f;

	AIJPArena* ArenaPtr = Arena.Get();
	if (!ArenaPtr)
	{
		return;
	}

	if (Input != 0.f)
	{
		LastMoveSign = FMath::Sign(Input);
	}

	const float MaxSpeed = GetMaxSpeed();
	if (DashTimeLeft > 0.f)
	{
		// Mid-dash: fixed burst speed. It ends at normal top speed, so the paddle doesn't slide on.
		Velocity = DashVelocity;
		DashTimeLeft -= DeltaSeconds;
		if (DashTimeLeft <= 0.f)
		{
			Velocity = FMath::Clamp(DashVelocity, -MaxSpeed, MaxSpeed);
		}
	}
	else
	{
		// Ramp toward the target speed at a constant rate, so reaching MaxSpeed from rest takes RampTime.
		const float RampTime = GetRampTime();
		const float TargetVelocity = Input * MaxSpeed;
		if (RampTime <= 0.f)
		{
			Velocity = TargetVelocity;
		}
		else
		{
			Velocity = FMath::FInterpConstantTo(Velocity, TargetVelocity, DeltaSeconds, MaxSpeed / RampTime);
		}
	}

	// Stay between the walls. Hitting a wall kills the velocity, so the paddle doesn't "push" into it.
	PlaneY += Velocity * DeltaSeconds;
	if (ClampToWalls())
	{
		Velocity = 0.f;
		DashTimeLeft = 0.f;
	}

	UpdateTransform();
}

void AIJPPaddle::UpdateTransform()
{
	const AIJPArena* ArenaPtr = Arena.Get();
	check(ArenaPtr);
	SetActorLocationAndRotation(ArenaPtr->PlaneToWorld(GetPlanePosition()), ArenaPtr->GetActorQuat());
}
