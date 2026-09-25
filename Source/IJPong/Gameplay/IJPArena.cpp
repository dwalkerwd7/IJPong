// It's Just Pong

#include "Gameplay/IJPArena.h"
#include "Gameplay/IJPHealthBarComponent.h"
#include "Narrative/IJPPortraitComponent.h"
#include "Presentation/IJPBackdropComponent.h"
#include "Abilities/IJPAbility_Split.h"
#include "Abilities/IJPAbilityComponent.h"
#include "Audio/IJPToneSet.h"
#include "Audio/IJPToneSynthComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/BoxComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Era/IJPEra.h"
#include "Era/IJPEraSubsystem.h"
#include "Gameplay/IJPBall.h"
#include "Gameplay/IJPBallType.h"
#include "Gameplay/IJPGoalComponent.h"
#include "Gameplay/IJPPaddle.h"
#include "Gameplay/IJPPaddleClass.h"
#include "Gameplay/IJPSevenSegmentComponent.h"
#include "Gameplay/IJPChargePipsComponent.h"
#include "Presentation/IJPCRTComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

AIJPArena::AIJPArena()
{
	PrimaryActorTick.bCanEverTick = false;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
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

	// Barriers exist from the start but block nothing and show nothing until raised.
	for (const EIJPSide Side : { EIJPSide::Left, EIJPSide::Right })
	{
		const bool bLeft = Side == EIJPSide::Left;
		UBoxComponent* Barrier = CreateDefaultSubobject<UBoxComponent>(bLeft ? TEXT("LeftBarrier") : TEXT("RightBarrier"));
		Barrier->SetupAttachment(Root);
		IJP::ConfigureAsBallBlocker(Barrier);
		Barrier->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		UStaticMeshComponent* BarrierVisual = CreateDefaultSubobject<UStaticMeshComponent>(bLeft ? TEXT("LeftBarrierVisual") : TEXT("RightBarrierVisual"));
		BarrierVisual->SetupAttachment(Root);
		BarrierVisual->SetStaticMesh(CubeMesh.Object);
		BarrierVisual->CastShadow = false;
		BarrierVisual->SetVisibility(false);
		IJP::ConfigureAsVisualOnly(BarrierVisual);
		(bLeft ? LeftBarrier : RightBarrier) = Barrier;
		(bLeft ? LeftBarrierVisual : RightBarrierVisual) = BarrierVisual;
	}

	LeftGoal = CreateDefaultSubobject<UIJPGoalComponent>(TEXT("LeftGoal"));
	LeftGoal->SetupAttachment(Root);
	LeftGoal->DefendingSide = EIJPSide::Left;

	RightGoal = CreateDefaultSubobject<UIJPGoalComponent>(TEXT("RightGoal"));
	RightGoal->SetupAttachment(Root);
	RightGoal->DefendingSide = EIJPSide::Right;

	Background = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Background"));
	Background->SetupAttachment(Root);
	Background->SetStaticMesh(CubeMesh.Object);
	Background->CastShadow = false;
	IJP::ConfigureAsVisualOnly(Background);

	WallVisuals = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("WallVisuals"));
	WallVisuals->SetupAttachment(Root);
	WallVisuals->SetStaticMesh(CubeMesh.Object);
	WallVisuals->CastShadow = false;
	IJP::ConfigureAsVisualOnly(WallVisuals);

	NetVisuals = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("NetVisuals"));
	NetVisuals->SetupAttachment(Root);
	NetVisuals->SetStaticMesh(CubeMesh.Object);
	NetVisuals->CastShadow = false;
	IJP::ConfigureAsVisualOnly(NetVisuals);

	LeftScore = CreateDefaultSubobject<UIJPSevenSegmentComponent>(TEXT("LeftScore"));
	LeftScore->SetupAttachment(Root);

	RightScore = CreateDefaultSubobject<UIJPSevenSegmentComponent>(TEXT("RightScore"));
	RightScore->SetupAttachment(Root);

	LeftHealthBar = CreateDefaultSubobject<UIJPHealthBarComponent>(TEXT("LeftHealthBar"));
	LeftHealthBar->SetupAttachment(Root);
	RightHealthBar = CreateDefaultSubobject<UIJPHealthBarComponent>(TEXT("RightHealthBar"));
	RightHealthBar->SetupAttachment(Root);
	RightHealthBar->SetRelativeScale3D(FVector(-1.f, 1.f, 1.f)); // Mirrored: drains toward the net too.

	Backdrop = CreateDefaultSubobject<UIJPBackdropComponent>(TEXT("Backdrop"));
	Backdrop->SetupAttachment(Root);

	LeftPortrait = CreateDefaultSubobject<UIJPPortraitComponent>(TEXT("LeftPortrait"));
	LeftPortrait->SetupAttachment(Root);
	RightPortrait = CreateDefaultSubobject<UIJPPortraitComponent>(TEXT("RightPortrait"));
	RightPortrait->SetupAttachment(Root);

	LeftPips = CreateDefaultSubobject<UIJPChargePipsComponent>(TEXT("LeftPips"));
	LeftPips->SetupAttachment(Root);
	RightPips = CreateDefaultSubobject<UIJPChargePipsComponent>(TEXT("RightPips"));
	RightPips->SetupAttachment(Root);

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

	// Barriers: full-height lines just inside each goal line, behind the paddles.
	const float BarrierX = HalfExtents.X - BarrierInset;
	const float CubeUnits = 100.f; // /Engine/BasicShapes/Cube is 100 units, centred.
	for (const EIJPSide Side : { EIJPSide::Left, EIJPSide::Right })
	{
		const bool bLeft = Side == EIJPSide::Left;
		const FVector Centre(IJP::SideSign(Side) * BarrierX, 0.f, 0.f);
		UBoxComponent* Barrier = bLeft ? LeftBarrier : RightBarrier;
		Barrier->SetRelativeLocation(Centre);
		Barrier->SetBoxExtent(FVector(BarrierThickness * 0.5f, HalfBlockerDepth, HalfExtents.Y));
		UStaticMeshComponent* BarrierVisual = bLeft ? LeftBarrierVisual : RightBarrierVisual;
		BarrierVisual->SetRelativeLocation(Centre);
		BarrierVisual->SetRelativeScale3D(FVector(BarrierThickness / CubeUnits, VisualDepth / CubeUnits, HalfExtents.Y * 2.f / CubeUnits));
	}

	// Goals start at the goal line and extend outward, covering the full height including walls.
	const float GoalCentreX = HalfExtents.X + GoalDepth * 0.5f;
	const FVector GoalExtent(GoalDepth * 0.5f, HalfBlockerDepth, OuterHalfY);
	LeftGoal->SetRelativeLocation(FVector(-GoalCentreX, 0.f, 0.f));
	LeftGoal->SetBoxExtent(GoalExtent);
	RightGoal->SetRelativeLocation(FVector(GoalCentreX, 0.f, 0.f));
	RightGoal->SetBoxExtent(GoalExtent);

	// The base material in the editor; BeginPlay swaps in the palette's instances.
	UMaterialInterface* BaseMaterial = PongMaterial.LoadSynchronous();
	for (UPrimitiveComponent* Piece : TArray<UPrimitiveComponent*>{ Background, WallVisuals, NetVisuals, LeftScore, RightScore, LeftBarrierVisual, RightBarrierVisual })
	{
		Piece->SetMaterial(0, BaseMaterial);
	}

	WallVisuals->ClearInstances();
	if (bShowWalls)
	{
		const FVector2D WallSize(HalfExtents.X * 2.f, WallThickness);
		AddVisualBox(WallVisuals, FVector2D(0.f, WallCentreY), WallSize);
		AddVisualBox(WallVisuals, FVector2D(0.f, -WallCentreY), WallSize);
	}

	// Net: dashes centred on the middle, stepping outward until they would touch a wall.
	NetVisuals->ClearInstances();
	const float Step = NetDashSize.Y + NetDashGap;
	for (float Y = 0.f; Y + NetDashSize.Y * 0.5f <= HalfExtents.Y; Y += Step)
	{
		AddVisualBox(NetVisuals, FVector2D(0.f, Y), NetDashSize);
		if (Y > 0.f)
		{
			AddVisualBox(NetVisuals, FVector2D(0.f, -Y), NetDashSize);
		}
	}

	LayoutHealth();

	// Frame the playfield plus walls and margin; width follows from the screen's aspect ratio.
	const float ScreenHeight = 2.f * (OuterHalfY + ScreenMargin);
	Camera->AspectRatio = ScreenAspectRatio;
	Camera->OrthoWidth = ScreenHeight * ScreenAspectRatio;

	// Background: a thin slab just behind every piece, filling exactly what the camera frames.
	const float CubeSize = 100.f; // /Engine/BasicShapes/Cube is 100 units, centred.
	Background->SetRelativeLocation(FVector(0.f, -VisualDepth, 0.f));
	Background->SetRelativeScale3D(FVector(Camera->OrthoWidth / CubeSize, 1.f / CubeSize, ScreenHeight / CubeSize));
}

void AIJPArena::BeginPlay()
{
	Super::BeginPlay();

	// Before the paddles and ball spawn, so they pick up the palette's materials.
	CreatePaletteMaterials();
	const UIJPEra* Era = UIJPEraSubsystem::GetCurrentEra(this);
	ApplyPalette(Era ? Era->Palette : FIJPPalette());
	if (UIJPEraSubsystem* Eras = UIJPEraSubsystem::Get(this))
	{
		Eras->OnEraChanged.AddDynamic(this, &AIJPArena::HandleEraChanged);
	}

	LeftPaddle = SpawnPaddle(EIJPSide::Left);
	RightPaddle = SpawnPaddle(EIJPSide::Right);
	LoadedToneSet = ToneSet.LoadSynchronous();
	DefaultBallType.LoadSynchronous();
	SpawnBall(GetDefaultBallType());
}

void AIJPArena::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UIJPEraSubsystem* Eras = UIJPEraSubsystem::Get(this))
	{
		Eras->OnEraChanged.RemoveAll(this);
	}

	// The arena spawned the paddles and balls, so it takes them with it.
	TArray<AActor*> Spawned = { LeftPaddle.Get(), RightPaddle.Get() };
	for (AIJPBall* Each : Balls)
	{
		Spawned.Add(Each);
	}
	for (AActor* Actor : Spawned)
	{
		if (IsValid(Actor))
		{
			Actor->Destroy();
		}
	}
	LeftPaddle = nullptr;
	RightPaddle = nullptr;
	Balls.Reset();

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
		const TSoftObjectPtr<UIJPPaddleClass>& SideClass = Side == EIJPSide::Left ? LeftPaddleClass : RightPaddleClass;
		Paddle->InitPaddle(this, Side, GetLaneX(Side), SideClass.LoadSynchronous());
	}
	return Paddle;
}

const UIJPToneSet& AIJPArena::GetToneSet() const
{
	if (LoadedToneSet)
	{
		return *LoadedToneSet;
	}
	const UIJPEra* Era = UIJPEraSubsystem::GetCurrentEra(this);
	return Era && Era->ToneSet ? *Era->ToneSet : *GetDefault<UIJPToneSet>();
}

UMaterialInterface* AIJPArena::GetPaletteMaterial(EIJPPaletteRole PaletteRole) const
{
	const int32 Index = static_cast<int32>(PaletteRole);
	return PaletteMaterials.IsValidIndex(Index) ? PaletteMaterials[Index].Get() : PongMaterial.Get();
}

void AIJPArena::CreatePaletteMaterials()
{
	PaletteMaterials.Reset();
	UMaterialInterface* Base = PongMaterial.LoadSynchronous();
	if (!Base)
	{
		UE_LOG(LogIJPong, Warning, TEXT("%s has no PongMaterial; the era's palette can't be shown."), *GetName());
		return;
	}

	for (int32 i = 0; i < static_cast<int32>(EIJPPaletteRole::Count); ++i)
	{
		PaletteMaterials.Add(UMaterialInstanceDynamic::Create(Base, this));
	}
	Background->SetMaterial(0, GetPaletteMaterial(EIJPPaletteRole::Background));
	WallVisuals->SetMaterial(0, GetPaletteMaterial(EIJPPaletteRole::Walls));
	NetVisuals->SetMaterial(0, GetPaletteMaterial(EIJPPaletteRole::Net));
	LeftScore->SetMaterial(0, GetPaletteMaterial(EIJPPaletteRole::Score));
	RightScore->SetMaterial(0, GetPaletteMaterial(EIJPPaletteRole::Score));
	LeftPips->SetMaterial(0, GetPaletteMaterial(EIJPPaletteRole::Score));
	RightPips->SetMaterial(0, GetPaletteMaterial(EIJPPaletteRole::Score));
	// A barrier is its paddle's, so it takes that paddle's colour.
	LeftBarrierVisual->SetMaterial(0, GetPaletteMaterial(EIJPPaletteRole::LeftPaddle));
	RightBarrierVisual->SetMaterial(0, GetPaletteMaterial(EIJPPaletteRole::RightPaddle));
}

void AIJPArena::SetBarrierUp(EIJPSide Side, bool bUp)
{
	const bool bLeft = Side == EIJPSide::Left;
	(bLeft ? LeftBarrier : RightBarrier)->SetCollisionEnabled(bUp ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
	(bLeft ? LeftBarrierVisual : RightBarrierVisual)->SetVisibility(bUp);
}

bool AIJPArena::ShowsSprites() const
{
	const UIJPEra* Era = UIJPEraSubsystem::GetCurrentEra(this);
	return Era && Era->bShowSprites;
}

UPrimitiveComponent* AIJPArena::GetBarrier(EIJPSide Side) const
{
	return Side == EIJPSide::Left ? LeftBarrier : RightBarrier;
}

bool AIJPArena::IsBarrierUp(EIJPSide Side) const
{
	return (Side == EIJPSide::Left ? LeftBarrier : RightBarrier)->GetCollisionEnabled() != ECollisionEnabled::NoCollision;
}

void AIJPArena::ApplyPalette(const FIJPPalette& Palette)
{
	CurrentPalette = Palette;
	static const FName ColorParam(TEXT("Color"));
	for (int32 i = 0; i < PaletteMaterials.Num(); ++i)
	{
		PaletteMaterials[i]->SetVectorParameterValue(ColorParam, Palette.Get(static_cast<EIJPPaletteRole>(i)));
	}

	// Each ball has its own material, since its colour can depend on its type; sprites follow the era too.
	for (AIJPBall* Each : Balls)
	{
		Each->RefreshColour();
	}
	for (const EIJPSide Side : { EIJPSide::Left, EIJPSide::Right })
	{
		if (AIJPPaddle* Paddle = GetPaddle(Side))
		{
			Paddle->RefreshSprite();
		}
	}
	RefreshHealthLook();
	Backdrop->SetShown(ShowsSprites());
}

void AIJPArena::SetBackdrop(const UIJPBackdrop* InBackdrop)
{
	const float ScreenHeight = Camera->OrthoWidth / FMath::Max(Camera->AspectRatio, 0.01f);
	Backdrop->SetBackdrop(InBackdrop, GetBackdropMaterial(), FVector2D(Camera->OrthoWidth, ScreenHeight));
	Backdrop->SetShown(ShowsSprites());
}

bool AIJPArena::ShowsHealthBar(EIJPSide Side) const
{
	return GetHealthBar(Side)->IsShown();
}

void AIJPArena::RefreshHealthLook()
{
	const UIJPEra* Era = UIJPEraSubsystem::GetCurrentEra(this);
	const FIJPHealthBarStyle Style = Era && Era->bShowSprites ? Era->HealthBar : FIJPHealthBarStyle();
	for (const EIJPSide Side : { EIJPSide::Left, EIJPSide::Right })
	{
		UIJPHealthBarComponent* Bar = GetHealthBar(Side);
		Bar->SetStyle(Style, GetSpriteMaterial());
		Bar->SetColours(CurrentPalette.Get(EIJPPaletteRole::Score), CurrentPalette.Get(Side == EIJPSide::Left ? EIJPPaletteRole::LeftPaddle : EIJPPaletteRole::RightPaddle));

		// A bar replaces the number only while there's health to show (endless matches count goals).
		Bar->SetShown(Style.IsSet() && HealthMax[Side == EIJPSide::Left ? 0 : 1] > 0.f);
		UIJPSevenSegmentComponent* Number = GetScoreDisplay(Side);
		if (Bar->IsShown())
		{
			Number->StopFlash();
		}
		Number->SetVisibility(!Bar->IsShown());

		GetPortrait(Side)->SetLook(Era && Era->bShowSprites, GetSpriteMaterial(), CurrentPalette.Get(Side == EIJPSide::Left ? EIJPPaletteRole::LeftPaddle : EIJPPaletteRole::RightPaddle));
	}
	LayoutHealth();
}

void AIJPArena::LayoutHealth()
{
	const float ScoreZ = HalfExtents.Y - ScoreOffset.Y - LeftScore->DigitSize.Y * 0.5f;
	LeftScore->SetRelativeLocation(FVector(-ScoreOffset.X, 0.f, ScoreZ));
	RightScore->SetRelativeLocation(FVector(ScoreOffset.X, 0.f, ScoreZ));
	for (const EIJPSide Side : { EIJPSide::Left, EIJPSide::Right })
	{
		const float Sign = IJP::SideSign(Side);
		UIJPHealthBarComponent* Bar = GetHealthBar(Side);
		UIJPChargePipsComponent* Pips = GetChargePips(Side);

		// The portrait sits at the top, just inside the bar's net end (or in the corner without a bar).
		UIJPPortraitComponent* Portrait = GetPortrait(Side);
		const float OuterEdge = Sign * (HalfExtents.X - HealthBarMargin.X);
		const float BarWidth = Bar->IsShown() ? Bar->GetSize().X + 8.f : 0.f;
		Portrait->SetRelativeLocation(FVector(OuterEdge - Sign * (BarWidth + Portrait->Size * 0.5f), 0.f, HalfExtents.Y - HealthBarMargin.Y - Portrait->Size * 0.5f));

		if (Bar->IsShown())
		{
			// The bar hugs its own side's top corner; the pips sit centred under it.
			const FVector2D Size = Bar->GetSize();
			const float BarZ = HalfExtents.Y - HealthBarMargin.Y - Size.Y * 0.5f;
			const float OuterX = Sign * (HalfExtents.X - HealthBarMargin.X);
			Bar->SetRelativeLocation(FVector(OuterX, 0.f, BarZ));
			Pips->SetRelativeLocation(FVector(OuterX - Sign * Size.X * 0.5f, 0.f, BarZ - Size.Y * 0.5f - 16.f));
		}
		else
		{
			Pips->SetRelativeLocation(FVector(Sign * ScoreOffset.X, 0.f, ScoreZ - LeftScore->DigitSize.Y * 0.5f - 20.f));
		}
	}
}

void AIJPArena::SetPortraits(EIJPSide Side, const FIJPPortraits& Portraits)
{
	GetPortrait(Side)->SetPortraits(Portraits);
}

void AIJPArena::UpdateMoods()
{
	const bool bRace = HealthMax[0] > 0.f && HealthMax[1] > 0.f;
	const float Lead = bRace ? HealthNow[0] / HealthMax[0] - HealthNow[1] / HealthMax[1] : 0.f;
	for (const EIJPSide Side : { EIJPSide::Left, EIJPSide::Right })
	{
		const float SideLead = Side == EIJPSide::Left ? Lead : -Lead;
		GetPortrait(Side)->SetMood(SideLead > KINDA_SMALL_NUMBER ? EIJPExpression::Smug : SideLead < -KINDA_SMALL_NUMBER ? EIJPExpression::Rattled : EIJPExpression::Neutral);
	}
}

void AIJPArena::SetHealthDisplay(EIJPSide Side, float Health, float MaxHealth, bool bInstant)
{
	// A hit (not a new match or a set health) shakes the scenery.
	if (!bInstant && Health < HealthNow[Side == EIJPSide::Left ? 0 : 1])
	{
		Backdrop->Shake();
	}
	HealthNow[Side == EIJPSide::Left ? 0 : 1] = Health;
	float& Max = HealthMax[Side == EIJPSide::Left ? 0 : 1];
	const bool bHadHealth = Max > 0.f;
	Max = FMath::Max(MaxHealth, 0.f);
	if (bHadHealth != (Max > 0.f))
	{
		RefreshHealthLook();
	}
	if (Max > 0.f)
	{
		GetHealthBar(Side)->SetFraction(Health / Max, bInstant);
	}
	UpdateMoods();
}

void AIJPArena::HandleEraChanged(const UIJPEra* NewEra)
{
	ApplyPalette(NewEra ? NewEra->Palette : FIJPPalette());
}

void AIJPArena::HandleBallPaddleHit(AIJPBall* HitBall, AIJPPaddle* Paddle)
{
	Tones->PlayTone(GetToneSet().PaddleHit);
	Paddle->Flicker();
	Paddle->GetAbilities()->HandleBallHit(*HitBall);

	// Twin: the first return splits it in two, and neither half splits again.
	const UIJPBallType& HitType = HitBall->GetType();
	if (HitType.bSplitsOnFirstHit && !HitBall->HasSplit() && HitBall->IsInPlay() && !HitBall->IsHeld())
	{
		HitBall->MarkSplit();
		if (AIJPBall* Half = UIJPAbility_Split::FanOut(*HitBall, *this, HitType.SplitSpread))
		{
			Half->MarkSplit();
		}
	}
	OnBallReturned.Broadcast(HitBall, Paddle);
}

void AIJPArena::HandleBallBounce()
{
	Tones->PlayTone(GetToneSet().Bounce);
}

void AIJPArena::HandleBallGoal(AIJPBall* ScoringBall, EIJPSide DefendingSide)
{
	Tones->PlayTone(GetToneSet().Goal);
	CRT->Pulse();

	// Bomb: the paddle it got past is stunned.
	const float Stun = ScoringBall ? ScoringBall->GetType().StunOnGoal : 0.f;
	if (AIJPPaddle* Beaten = Stun > 0.f ? GetPaddle(DefendingSide) : nullptr)
	{
		Beaten->Stun(Stun);
	}
	OnBallGoal.Broadcast(ScoringBall, DefendingSide);
}

AIJPBall* AIJPArena::SpawnBall(const UIJPBallType* Type)
{
	if (!BallClass)
	{
		UE_LOG(LogIJPong, Error, TEXT("%s has no BallClass; no ball spawned."), *GetName());
		return nullptr;
	}

	FActorSpawnParameters Params;
	Params.Owner = this;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AIJPBall* NewBall = GetWorld()->SpawnActor<AIJPBall>(BallClass, GetActorTransform(), Params);
	if (NewBall)
	{
		NewBall->InitBall(this, Type);
		NewBall->OnPaddleHit.AddDynamic(this, &AIJPArena::HandleBallPaddleHit);
		NewBall->OnBounce.AddDynamic(this, &AIJPArena::HandleBallBounce);
		NewBall->OnGoal.AddDynamic(this, &AIJPArena::HandleBallGoal);
		Balls.Add(NewBall);
	}
	return NewBall;
}

AIJPBall* AIJPArena::AddBall(const UIJPBallType* Type)
{
	if (!Type)
	{
		Type = GetDefaultBallType();
	}

	// Reuse a ball that's waiting (not in play, not blinking for a serve) before spawning another.
	for (AIJPBall* Existing : Balls)
	{
		if (!Existing->IsInPlay() && !Existing->IsBlinking())
		{
			Existing->ResetBall();
			Existing->SetType(Type);
			return Existing;
		}
	}
	return SpawnBall(Type);
}

int32 AIJPArena::GetNumBallsInPlay() const
{
	int32 Count = 0;
	for (const AIJPBall* Each : Balls)
	{
		Count += Each->IsInPlay() ? 1 : 0;
	}
	return Count;
}

void AIJPArena::ResetBalls()
{
	for (AIJPBall* Each : Balls)
	{
		Each->ResetBall();
	}
}

const UIJPBallType* AIJPArena::GetDefaultBallType() const
{
	return DefaultBallType.Get() ? DefaultBallType.Get() : GetDefault<UIJPBallType>();
}

FLinearColor AIJPArena::GetBallColour(const UIJPBallType* Type) const
{
	return CurrentPalette.bBallTypeColours && Type ? Type->Colour : CurrentPalette.Ball;
}

void AIJPArena::FlashScore(EIJPSide Side)
{
	// A bar shows the hit with its trail instead.
	if (!ShowsHealthBar(Side))
	{
		GetScoreDisplay(Side)->Flash();
	}
}

void AIJPArena::ShowWinner(EIJPSide Winner)
{
	const EIJPSide Loser = IJP::Opposite(Winner);
	if (ShowsHealthBar(Winner))
	{
		GetHealthBar(Loser)->StopFlash();
		GetHealthBar(Winner)->Flash(0, WinnerBlinkPeriod);
		return;
	}
	GetScoreDisplay(Loser)->StopFlash();
	GetScoreDisplay(Winner)->Flash(0, WinnerBlinkPeriod);
}

void AIJPArena::ClearWinner()
{
	for (const EIJPSide Side : { EIJPSide::Left, EIJPSide::Right })
	{
		GetHealthBar(Side)->StopFlash();
		if (!ShowsHealthBar(Side))
		{
			GetScoreDisplay(Side)->StopFlash();
		}
	}
}

void AIJPArena::SetPaddleClass(EIJPSide Side, const UIJPPaddleClass* SideClass)
{
	if (AIJPPaddle* Paddle = GetPaddle(Side))
	{
		Paddle->SetPaddleClass(SideClass);
	}
	else
	{
		(Side == EIJPSide::Left ? LeftPaddleClass : RightPaddleClass) = const_cast<UIJPPaddleClass*>(SideClass);
	}
}

const UIJPPaddleClass* AIJPArena::GetConfiguredPaddleClass(EIJPSide Side) const
{
	return (Side == EIJPSide::Left ? LeftPaddleClass : RightPaddleClass).LoadSynchronous();
}

AIJPPaddle* AIJPArena::GetPaddle(EIJPSide Side) const
{
	return Side == EIJPSide::Left ? LeftPaddle : RightPaddle;
}

float AIJPArena::GetLaneX(EIJPSide Side) const
{
	return IJP::SideSign(Side) * (HalfExtents.X - PaddleInset);
}

void AIJPArena::SetChargePips(EIJPSide Side, int32 Filled, int32 Total)
{
	GetChargePips(Side)->SetCharge(Filled, Total);
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

void AIJPArena::AddVisualBox(UInstancedStaticMeshComponent* Target, const FVector2D& Centre, const FVector2D& Size)
{
	const float CubeSize = 100.f; // /Engine/BasicShapes/Cube is 100 units, centred.
	const FVector Scale(Size.X / CubeSize, VisualDepth / CubeSize, Size.Y / CubeSize);
	Target->AddInstance(FTransform(FQuat::Identity, FVector(Centre.X, 0.f, Centre.Y), Scale));
}
