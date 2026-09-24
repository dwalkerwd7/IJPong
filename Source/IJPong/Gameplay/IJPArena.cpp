// It's Just Pong

#include "Gameplay/IJPArena.h"
#include "Audio/IJPToneSet.h"
#include "Audio/IJPToneSynthComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/BoxComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Gameplay/IJPBall.h"
#include "Gameplay/IJPGoalComponent.h"
#include "Gameplay/IJPPaddle.h"
#include "Gameplay/IJPPaddleProfile.h"
#include "Gameplay/IJPSevenSegmentComponent.h"
#include "Presentation/IJPCRTComponent.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

AIJPArena::AIJPArena()
{
	PrimaryActorTick.bCanEverTick = false;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> UnlitMaterial(TEXT("/Engine/EngineMaterials/EmissiveMeshMaterial.EmissiveMeshMaterial"));
	PongMaterial = UnlitMaterial.Object;
	PaddleClass = AIJPPaddle::StaticClass();
	BallClass = AIJPBall::StaticClass();

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	TopWall = CreateDefaultSubobject<UBoxComponent>(TEXT("TopWall"));
	TopWall->SetupAttachment(Root);
	IJP::ConfigureAsBallBlocker(TopWall);

	BottomWall = CreateDefaultSubobject<UBoxComponent>(TEXT("BottomWall"));
	BottomWall->SetupAttachment(Root);
	IJP::ConfigureAsBallBlocker(BottomWall);

	LeftGoal = CreateDefaultSubobject<UIJPGoalComponent>(TEXT("LeftGoal"));
	LeftGoal->SetupAttachment(Root);
	LeftGoal->DefendingSide = EIJPSide::Left;

	RightGoal = CreateDefaultSubobject<UIJPGoalComponent>(TEXT("RightGoal"));
	RightGoal->SetupAttachment(Root);
	RightGoal->DefendingSide = EIJPSide::Right;

	Visuals = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Visuals"));
	Visuals->SetupAttachment(Root);
	Visuals->SetStaticMesh(CubeMesh.Object);
	Visuals->CastShadow = false;
	IJP::ConfigureAsVisualOnly(Visuals);

	LeftScore = CreateDefaultSubobject<UIJPSevenSegmentComponent>(TEXT("LeftScore"));
	LeftScore->SetupAttachment(Root);

	RightScore = CreateDefaultSubobject<UIJPSevenSegmentComponent>(TEXT("RightScore"));
	RightScore->SetupAttachment(Root);

	CRT = CreateDefaultSubobject<UIJPCRTComponent>(TEXT("CRT"));

	Tones = CreateDefaultSubobject<UIJPToneSynthComponent>(TEXT("Tones"));
	Tones->SetupAttachment(Root);

	// Sits on plane-space "front" (+Y) looking back at the playfield: screen right = +X, screen up = +Z.
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(Root);
	Camera->SetRelativeLocationAndRotation(FVector(0.f, 1000.f, 0.f), FRotator(0.f, -90.f, 0.f));
	Camera->ProjectionMode = ECameraProjectionMode::Orthographic;
	Camera->bConstrainAspectRatio = true;

	// Flat, crisp output: fixed exposure and no lens effects, so white stays white.
	FPostProcessSettings& PP = Camera->PostProcessSettings;
	PP.bOverride_AutoExposureMethod = true;
	PP.AutoExposureMethod = AEM_Manual;
	PP.bOverride_AutoExposureApplyPhysicalCameraExposure = true;
	PP.AutoExposureApplyPhysicalCameraExposure = false;
	PP.bOverride_AutoExposureBias = true;
	PP.AutoExposureBias = 0.f;
	PP.bOverride_BloomIntensity = true;
	PP.BloomIntensity = 0.f;
	PP.bOverride_MotionBlurAmount = true;
	PP.MotionBlurAmount = 0.f;
	PP.bOverride_VignetteIntensity = true;
	PP.VignetteIntensity = 0.f;
	PP.bOverride_LensFlareIntensity = true;
	PP.LensFlareIntensity = 0.f;
}

void AIJPArena::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	const float HalfBlockerDepth = BlockerDepth * 0.5f;
	const float WallCentreY = HalfExtents.Y + WallThickness * 0.5f;
	const float OuterHalfY = HalfExtents.Y + WallThickness;

	// Walls: the ball's collision always exists; the visible line is optional.
	TopWall->SetRelativeLocation(FVector(0.f, 0.f, WallCentreY));
	TopWall->SetBoxExtent(FVector(HalfExtents.X + GoalDepth, HalfBlockerDepth, WallThickness * 0.5f));
	BottomWall->SetRelativeLocation(FVector(0.f, 0.f, -WallCentreY));
	BottomWall->SetBoxExtent(FVector(HalfExtents.X + GoalDepth, HalfBlockerDepth, WallThickness * 0.5f));

	// Goals start at the goal line and extend outward, covering the full height including walls.
	const float GoalCentreX = HalfExtents.X + GoalDepth * 0.5f;
	const FVector GoalExtent(GoalDepth * 0.5f, HalfBlockerDepth, OuterHalfY);
	LeftGoal->SetRelativeLocation(FVector(-GoalCentreX, 0.f, 0.f));
	LeftGoal->SetBoxExtent(GoalExtent);
	RightGoal->SetRelativeLocation(FVector(GoalCentreX, 0.f, 0.f));
	RightGoal->SetBoxExtent(GoalExtent);

	Visuals->ClearInstances();
	Visuals->SetMaterial(0, PongMaterial);
	if (bShowWalls)
	{
		const FVector2D WallSize(HalfExtents.X * 2.f, WallThickness);
		AddVisualBox(FVector2D(0.f, WallCentreY), WallSize);
		AddVisualBox(FVector2D(0.f, -WallCentreY), WallSize);
	}

	// Net: dashes centred on the middle, stepping outward until they would touch a wall.
	const float Step = NetDashSize.Y + NetDashGap;
	for (float Y = 0.f; Y + NetDashSize.Y * 0.5f <= HalfExtents.Y; Y += Step)
	{
		AddVisualBox(FVector2D(0.f, Y), NetDashSize);
		if (Y > 0.f)
		{
			AddVisualBox(FVector2D(0.f, -Y), NetDashSize);
		}
	}

	const float ScoreZ = HalfExtents.Y - ScoreOffset.Y - LeftScore->DigitSize.Y * 0.5f;
	LeftScore->SetRelativeLocation(FVector(-ScoreOffset.X, 0.f, ScoreZ));
	LeftScore->SetMaterial(0, PongMaterial);
	RightScore->SetRelativeLocation(FVector(ScoreOffset.X, 0.f, ScoreZ));
	RightScore->SetMaterial(0, PongMaterial);

	// Frame the playfield plus walls and margin; width follows from the screen's aspect ratio.
	const float ScreenHeight = 2.f * (OuterHalfY + ScreenMargin);
	Camera->AspectRatio = ScreenAspectRatio;
	Camera->OrthoWidth = ScreenHeight * ScreenAspectRatio;
}

void AIJPArena::BeginPlay()
{
	Super::BeginPlay();

	LeftPaddle = SpawnPaddle(EIJPSide::Left);
	RightPaddle = SpawnPaddle(EIJPSide::Right);
	LoadedToneSet = ToneSet.LoadSynchronous();

	if (BallClass)
	{
		FActorSpawnParameters Params;
		Params.Owner = this;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Ball = GetWorld()->SpawnActor<AIJPBall>(BallClass, GetActorTransform(), Params);
		if (Ball)
		{
			Ball->InitBall(this);
			Ball->OnPaddleHit.AddDynamic(this, &AIJPArena::HandleBallPaddleHit);
			Ball->OnBounce.AddDynamic(this, &AIJPArena::HandleBallBounce);
			Ball->OnGoal.AddDynamic(this, &AIJPArena::HandleBallGoal);
		}
	}
	else
	{
		UE_LOG(LogIJPong, Error, TEXT("%s has no BallClass; no ball spawned."), *GetName());
	}
}

void AIJPArena::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// The arena spawned the paddles and ball, so it takes them with it.
	for (AActor* Spawned : { static_cast<AActor*>(LeftPaddle.Get()), static_cast<AActor*>(RightPaddle.Get()), static_cast<AActor*>(Ball.Get()) })
	{
		if (IsValid(Spawned))
		{
			Spawned->Destroy();
		}
	}
	LeftPaddle = nullptr;
	RightPaddle = nullptr;
	Ball = nullptr;

	Super::EndPlay(EndPlayReason);
}

AIJPPaddle* AIJPArena::SpawnPaddle(EIJPSide Side)
{
	if (!PaddleClass)
	{
		UE_LOG(LogIJPong, Error, TEXT("%s has no PaddleClass; no paddles spawned."), *GetName());
		return nullptr;
	}

	FActorSpawnParameters Params;
	Params.Owner = this;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AIJPPaddle* Paddle = GetWorld()->SpawnActor<AIJPPaddle>(PaddleClass, GetActorTransform(), Params);
	if (Paddle)
	{
		const TSoftObjectPtr<UIJPPaddleProfile>& Profile = Side == EIJPSide::Left ? LeftPaddleProfile : RightPaddleProfile;
		Paddle->InitPaddle(this, Side, GetLaneX(Side), Profile.LoadSynchronous());
	}
	return Paddle;
}

const UIJPToneSet& AIJPArena::GetToneSet() const
{
	return LoadedToneSet ? *LoadedToneSet : *GetDefault<UIJPToneSet>();
}

void AIJPArena::HandleBallPaddleHit(AIJPPaddle* Paddle)
{
	Tones->PlayTone(GetToneSet().PaddleHit);
	Paddle->Flicker();
}

void AIJPArena::HandleBallBounce()
{
	Tones->PlayTone(GetToneSet().Bounce);
}

void AIJPArena::HandleBallGoal(EIJPSide DefendingSide)
{
	Tones->PlayTone(GetToneSet().Goal);
	CRT->Pulse();
}

void AIJPArena::FlashScore(EIJPSide Side)
{
	GetScoreDisplay(Side)->Flash();
}

void AIJPArena::SetPaddleProfile(EIJPSide Side, UIJPPaddleProfile* Profile)
{
	(Side == EIJPSide::Left ? LeftPaddleProfile : RightPaddleProfile) = Profile;
}

AIJPPaddle* AIJPArena::GetPaddle(EIJPSide Side) const
{
	return Side == EIJPSide::Left ? LeftPaddle : RightPaddle;
}

float AIJPArena::GetLaneX(EIJPSide Side) const
{
	return IJP::SideSign(Side) * (HalfExtents.X - PaddleInset);
}

void AIJPArena::SetScore(EIJPSide Side, int32 Score)
{
	(Side == EIJPSide::Left ? LeftScore : RightScore)->SetValue(Score);
}

FTransform AIJPArena::GetPlaneTransform() const
{
	// Plane space ignores actor scale so gameplay distances stay in world units.
	FTransform PlaneTransform = GetActorTransform();
	PlaneTransform.SetScale3D(FVector::OneVector);
	return PlaneTransform;
}

FVector AIJPArena::PlaneToWorld(const FVector2D& PlanePoint) const
{
	return GetPlaneTransform().TransformPosition(FVector(PlanePoint.X, 0.f, PlanePoint.Y));
}

FVector2D AIJPArena::WorldToPlane(const FVector& WorldPoint) const
{
	const FVector Local = GetPlaneTransform().InverseTransformPosition(WorldPoint);
	return FVector2D(Local.X, Local.Z);
}

FVector AIJPArena::PlaneDirToWorld(const FVector2D& PlaneDir) const
{
	return GetActorQuat().RotateVector(FVector(PlaneDir.X, 0.f, PlaneDir.Y));
}

FVector2D AIJPArena::WorldDirToPlane(const FVector& WorldDir) const
{
	const FVector Local = GetActorQuat().UnrotateVector(WorldDir);
	return FVector2D(Local.X, Local.Z);
}

void AIJPArena::AddVisualBox(const FVector2D& Centre, const FVector2D& Size)
{
	const float CubeSize = 100.f; // /Engine/BasicShapes/Cube is 100 units, centred.
	const FVector Scale(Size.X / CubeSize, VisualDepth / CubeSize, Size.Y / CubeSize);
	Visuals->AddInstance(FTransform(FQuat::Identity, FVector(Centre.X, 0.f, Centre.Y), Scale));
}
