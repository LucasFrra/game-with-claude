// Project Eldoria

#include "Bosses/EldoriaTharum.h"
#include "Bosses/EldoriaControlDevice.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	constexpr int32 TrunkSegments = 7;
	constexpr float TrunkSegmentLength = 130.f;
	const FName DeityCollisionProfile(TEXT("BlockAllDynamic"));

	// FL, FR, RL, RR : four-beat lateral gait
	const float LegPhaseOffsets[4] = { HALF_PI, HALF_PI * 3.f, 0.f, PI };
	const FVector HipLocations[4] = { FVector(650.f, -360.f, 660.f), FVector(650.f, 360.f, 660.f), FVector(-650.f, -380.f, 660.f), FVector(-650.f, 380.f, 660.f) };
}

UStaticMeshComponent* AEldoriaTharum::MakeMesh(FName Name, USceneComponent* Parent, UStaticMesh* Mesh, const FVector& Location, const FVector& Scale, const FRotator& Rotation)
{
	UStaticMeshComponent* Component = CreateDefaultSubobject<UStaticMeshComponent>(Name);
	Component->SetupAttachment(Parent);
	Component->SetStaticMesh(Mesh);
	Component->SetRelativeLocation(Location);
	Component->SetRelativeRotation(Rotation);
	Component->SetRelativeScale3D(Scale);
	Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Component->SetCanEverAffectNavigation(false);
	return Component;
}

AEldoriaTharum::AEldoriaTharum()
{
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cone(TEXT("/Engine/BasicShapes/Cone.Cone"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));

	// Sculpted parts (SourceArt/Scripts/blender_eldoria.py), modelled at full size in metres.
	// "Unit" parts (trunk, tusks, armor) are 1 m like the basic shapes and keep their scales.
#define ELDORIA_THARUM_MESH(Var, Name) static ConstructorHelpers::FObjectFinder<UStaticMesh> Var(TEXT("/Game/Project/Bosses/Tharum/Meshes/" Name "/StaticMeshes/" Name "." Name))
	ELDORIA_THARUM_MESH(BodyArt, "SM_Tharum_Body");
	ELDORIA_THARUM_MESH(HeadArt, "SM_Tharum_Head");
	ELDORIA_THARUM_MESH(EarAArt, "SM_Tharum_EarA");
	ELDORIA_THARUM_MESH(EarBArt, "SM_Tharum_EarB");
	ELDORIA_THARUM_MESH(LegArt, "SM_Tharum_Leg");
	ELDORIA_THARUM_MESH(FootArt, "SM_Tharum_Foot");
	ELDORIA_THARUM_MESH(TrunkArt, "SM_Tharum_TrunkSegment");
	ELDORIA_THARUM_MESH(TuskArt, "SM_Tharum_Tusk");
	ELDORIA_THARUM_MESH(TuskTipArt, "SM_Tharum_TuskTip");
	ELDORIA_THARUM_MESH(ArmorArt, "SM_Tharum_Armor");
#undef ELDORIA_THARUM_MESH
	// Falls back to basic shapes (with their scales) if an asset is missing
	auto Pick = [](const auto& Art, const auto& Fallback) { return Art.Succeeded() ? Art.Object : Fallback.Object; };
	auto Fit = [](const auto& Art, const FVector& FallbackScale) { return Art.Succeeded() ? FVector::OneVector : FallbackScale; };

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;
	BodyRoot = CreateDefaultSubobject<USceneComponent>(TEXT("BodyRoot"));
	BodyRoot->SetupAttachment(Root);

	// --- Body (all sizes in cm: basic shapes are 100 cm wide) ---
	// The sculpted body's flat saddle (4.2 m above its centre) sits just under the back platform
	BodyMesh = MakeMesh(TEXT("Body"), BodyRoot, Pick(BodyArt, Sphere), BodyArt.Succeeded() ? FVector(-80.f, 0.f, 955.f) : FVector(0.f, 0.f, 930.f), Fit(BodyArt, FVector(21.f, 10.5f, 9.f)));
	BellyMesh = MakeMesh(TEXT("Belly"), BodyRoot, Sphere.Object, FVector(120.f, 0.f, 760.f), FVector(16.f, 9.4f, 6.5f));
	if (BodyArt.Succeeded())
	{
		BellyMesh->SetVisibility(false);
		BellyMesh->SetHiddenInGame(true);
	}

	HeadPivot = CreateDefaultSubobject<USceneComponent>(TEXT("HeadPivot"));
	HeadPivot->SetupAttachment(BodyRoot);
	HeadPivot->SetRelativeLocation(FVector(950.f, 0.f, 1080.f));
	HeadMesh = MakeMesh(TEXT("Head"), HeadPivot, Pick(HeadArt, Sphere), FVector(HeadArt.Succeeded() ? 200.f : 300.f, 0.f, 0.f), Fit(HeadArt, FVector(6.6f, 6.f, 6.8f)));

	for (int32 Side = 0; Side < 2; ++Side)
	{
		const float Sign = Side == 0 ? -1.f : 1.f;
		USceneComponent* EarPivot = CreateDefaultSubobject<USceneComponent>(*FString::Printf(TEXT("EarPivot%d"), Side));
		EarPivot->SetupAttachment(HeadPivot);
		EarPivot->SetRelativeLocation(FVector(180.f, Sign * 260.f, 60.f));
		EarPivots.Add(EarPivot);
		const auto& EarArt = Side == 0 ? EarBArt : EarAArt;
		EarMeshes.Add(MakeMesh(*FString::Printf(TEXT("Ear%d"), Side), EarPivot, Pick(EarArt, Sphere), FVector(-60.f, Sign * 270.f, -40.f), Fit(EarArt, FVector(0.45f, 5.6f, 5.2f))));

		// Tusks: forward and down, then the tip curves up and slightly outwards
		const FRotator TuskRot(-110.f, Sign * 10.f, 0.f);
		const FVector TuskStart(420.f, Sign * 150.f, -170.f);
		const FVector TuskDir = TuskRot.RotateVector(FVector::UpVector);
		TuskMeshes.Add(MakeMesh(*FString::Printf(TEXT("Tusk%d"), Side), HeadPivot, Pick(TuskArt, Cylinder), TuskStart + TuskDir * 260.f, FVector(0.62f, 0.62f, 5.2f), TuskRot));
		const FRotator TipRot(-55.f, Sign * 14.f, 0.f);
		const FVector TipDir = TipRot.RotateVector(FVector::UpVector);
		TuskMeshes.Add(MakeMesh(*FString::Printf(TEXT("TuskTip%d"), Side), HeadPivot, Pick(TuskTipArt, Cone), TuskStart + TuskDir * 520.f + TipDir * 110.f, FVector(0.6f, 0.6f, 2.2f), TipRot));
	}

	// Trunk: a chain of pivots, each segment hanging along its pivot's -Z
	USceneComponent* Parent = HeadPivot;
	FVector Offset(560.f, 0.f, -120.f);
	for (int32 Index = 0; Index < TrunkSegments; ++Index)
	{
		USceneComponent* Pivot = CreateDefaultSubobject<USceneComponent>(*FString::Printf(TEXT("TrunkPivot%d"), Index));
		Pivot->SetupAttachment(Parent);
		Pivot->SetRelativeLocation(Offset);
		TrunkPivots.Add(Pivot);
		const float Radius = FMath::Lerp(80.f, 32.f, Index / float(TrunkSegments - 1));
		TrunkMeshes.Add(MakeMesh(*FString::Printf(TEXT("Trunk%d"), Index), Pivot, Pick(TrunkArt, Cylinder), FVector(0.f, 0.f, -TrunkSegmentLength * 0.5f), FVector(Radius / 50.f, Radius / 50.f, (TrunkSegmentLength + 20.f) / 100.f)));
		Parent = Pivot;
		Offset = FVector(0.f, 0.f, -TrunkSegmentLength);
	}

	// Legs
	for (int32 Index = 0; Index < 4; ++Index)
	{
		USceneComponent* Hip = CreateDefaultSubobject<USceneComponent>(*FString::Printf(TEXT("LegPivot%d"), Index));
		Hip->SetupAttachment(BodyRoot);
		Hip->SetRelativeLocation(HipLocations[Index]);
		LegPivots.Add(Hip);
		LegMeshes.Add(MakeMesh(*FString::Printf(TEXT("Leg%d"), Index), Hip, Pick(LegArt, Cylinder), FVector(0.f, 0.f, -300.f), Fit(LegArt, FVector(3.1f, 3.1f, 6.2f))));
		FootMeshes.Add(MakeMesh(*FString::Printf(TEXT("Foot%d"), Index), Hip, Pick(FootArt, Cylinder), FVector(0.f, 0.f, -615.f), Fit(FootArt, FVector(3.9f, 3.9f, 0.9f))));

		UCapsuleComponent* LegCollider = CreateDefaultSubobject<UCapsuleComponent>(*FString::Printf(TEXT("LegCollider%d"), Index));
		LegCollider->SetupAttachment(Hip);
		LegCollider->SetRelativeLocation(FVector(0.f, 0.f, -330.f));
		LegCollider->InitCapsuleSize(175.f, 330.f);
		LegCollider->SetCollisionProfileName(DeityCollisionProfile);
		LegColliders.Add(LegCollider);
	}

	// Ancient natural armor
	ArmorMeshes.Add(MakeMesh(TEXT("ArmorBack"), BodyRoot, Pick(ArmorArt, Cube), FVector(-120.f, 0.f, 1360.f), FVector(9.5f, 5.4f, 0.5f)));
	ArmorMeshes.Add(MakeMesh(TEXT("ArmorShoulderL"), BodyRoot, Pick(ArmorArt, Cube), FVector(520.f, -400.f, 1250.f), FVector(4.f, 3.f, 0.6f), FRotator(0.f, 0.f, -28.f)));
	ArmorMeshes.Add(MakeMesh(TEXT("ArmorShoulderR"), BodyRoot, Pick(ArmorArt, Cube), FVector(520.f, 400.f, 1250.f), FVector(4.f, 3.f, 0.6f), FRotator(0.f, 0.f, 28.f)));
	ArmorMeshes.Add(MakeMesh(TEXT("ArmorForehead"), HeadPivot, Pick(ArmorArt, Cube), FVector(470.f, 0.f, 250.f), FVector(2.6f, 3.6f, 0.6f), FRotator(-24.f, 0.f, 0.f)));

	TailMesh = MakeMesh(TEXT("Tail"), BodyRoot, Cylinder.Object, FVector(-1080.f, 0.f, 760.f), FVector(0.5f, 0.5f, 3.2f), FRotator(-12.f, 0.f, 0.f));

	DeviceSocket = CreateDefaultSubobject<USceneComponent>(TEXT("DeviceSocket"));
	DeviceSocket->SetupAttachment(BodyRoot);
	DeviceSocket->SetRelativeLocation(FVector(-250.f, 0.f, 1385.f));

	// --- Gameplay collision (separate from visuals) ---
	BodyCollider = CreateDefaultSubobject<UCapsuleComponent>(TEXT("BodyCollider"));
	BodyCollider->SetupAttachment(BodyRoot);
	BodyCollider->SetRelativeLocation(FVector(-80.f, 0.f, 930.f));
	BodyCollider->SetRelativeRotation(FRotator(90.f, 0.f, 0.f));
	BodyCollider->InitCapsuleSize(470.f, 850.f);	// as long as the sculpted body (17 m)
	BodyCollider->SetCollisionProfileName(DeityCollisionProfile);

	BackPlatform = CreateDefaultSubobject<UBoxComponent>(TEXT("BackPlatform"));
	BackPlatform->SetupAttachment(BodyRoot);
	BackPlatform->SetRelativeLocation(FVector(-120.f, 0.f, 1360.f));
	BackPlatform->InitBoxExtent(FVector(500.f, 420.f, 25.f));	// the sculpted saddle is ~8.5 m wide
	BackPlatform->SetCollisionProfileName(DeityCollisionProfile);
	BackPlatform->CanCharacterStepUpOn = ECB_Yes;

	HeadCollider = CreateDefaultSubobject<USphereComponent>(TEXT("HeadCollider"));
	HeadCollider->SetupAttachment(HeadPivot);
	HeadCollider->SetRelativeLocation(FVector(220.f, 0.f, 0.f));
	HeadCollider->InitSphereRadius(330.f);
	HeadCollider->SetCollisionProfileName(DeityCollisionProfile);
	// A toppling tower must be able to come down over the dazed head and rest on the back (route A ramp)
	HeadCollider->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Ignore);

	// Ground in front of the lowered head (local z = HeavyKneelDepth) up to the front of the back platform:
	// from (2000, 600) to (380, 1415) in body space, ~27 degrees
	NeckRamp = CreateDefaultSubobject<UBoxComponent>(TEXT("NeckRamp"));
	NeckRamp->SetupAttachment(BodyRoot);
	{
		const FVector Low(2000.f, 0.f, 600.f), High(380.f, 0.f, 1415.f);
		const FVector Along = Low - High;
		NeckRamp->SetRelativeLocation((Low + High) * 0.5f - FVector(0.f, 0.f, 15.f));
		NeckRamp->SetRelativeRotation(Along.Rotation());
		NeckRamp->InitBoxExtent(FVector(Along.Size() * 0.5f, 230.f, 15.f));
	}
	NeckRamp->SetCollisionProfileName(DeityCollisionProfile);
	NeckRamp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	NeckRamp->CanCharacterStepUpOn = ECB_Yes;
	NeckRamp->SetCanEverAffectNavigation(false);

	ControlDeviceClass = AEldoriaControlDevice::StaticClass();
}

void AEldoriaTharum::SetNeckRampActive(bool bActive)
{
	if (bActive == bNeckRampActive)
	{
		return;
	}
	bNeckRampActive = bActive;
	NeckRamp->SetCollisionEnabled(bActive ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
	// The head lies under the ramp: it must not bump the climbing player
	HeadCollider->SetCollisionResponseToChannel(ECC_Pawn, bActive ? ECR_Ignore : ECR_Block);
}

void AEldoriaTharum::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	auto Apply = [](UStaticMeshComponent* Component, UMaterialInterface* Material)
	{
		if (Component && Material)
		{
			Component->SetMaterial(0, Material);
		}
	};

	for (UStaticMeshComponent* Component : { BodyMesh.Get(), BellyMesh.Get(), HeadMesh.Get(), TailMesh.Get() })
	{
		Apply(Component, SkinMaterial);
	}
	for (const TArray<TObjectPtr<UStaticMeshComponent>>* List : { &EarMeshes, &TrunkMeshes, &LegMeshes, &FootMeshes })
	{
		for (UStaticMeshComponent* Component : *List)
		{
			Apply(Component, SkinMaterial);
		}
	}
	for (UStaticMeshComponent* Component : TuskMeshes)
	{
		Apply(Component, TuskMaterial);
	}
	for (UStaticMeshComponent* Component : FootMeshes)
	{
		// Sculpted feet have a second slot for the toenails
		if (Component && TuskMaterial && Component->GetNumMaterials() > 1)
		{
			Component->SetMaterial(1, TuskMaterial);
		}
	}
	for (UStaticMeshComponent* Component : ArmorMeshes)
	{
		Apply(Component, ArmorMaterial);
	}
}

void AEldoriaTharum::BeginPlay()
{
	Super::BeginPlay();

	if (ControlDeviceClass)
	{
		FActorSpawnParameters Params;
		Params.Owner = this;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		ControlDevice = GetWorld()->SpawnActor<AEldoriaControlDevice>(ControlDeviceClass, DeviceSocket->GetComponentTransform(), Params);
		if (ControlDevice)
		{
			ControlDevice->AttachToComponent(DeviceSocket, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		}
	}
}

void AEldoriaTharum::UpdateBodyAnimation(float DeltaSeconds)
{
	Clock += DeltaSeconds;
	GaitPhase += DeltaSeconds * (CurrentSpeed / StrideLength) * TWO_PI;

	const float Moving = FMath::Clamp(CurrentSpeed / FMath::Max(WalkSpeed, 1.f), 0.f, 1.6f);

	// Targets driven by the state
	float TargetZ = 0.f, TargetPitch = 0.f, TargetRoll = 0.f, TargetHead = 2.f * FMath::Sin(Clock * 0.7f);
	float TargetCurl = 9.f, TargetEars = 6.f, TargetKneel = 0.f, Paw = 0.f;

	switch (State)
	{
	case EEldoriaDeityState::PreparingCharge:
	{
		const float T = StateTime / FMath::Max(PrepareChargeTime, 0.1f);
		if (T < 0.45f)
		{
			// Rears back, raises the trunk, ears wide, scrapes the ground: unmistakable warning
			TargetPitch = 4.f; TargetHead = 10.f; TargetCurl = 24.f; TargetEars = 30.f;
			Paw = 28.f * FMath::Max(0.f, FMath::Sin(Clock * 9.f));
		}
		else
		{
			TargetPitch = -4.f; TargetHead = -15.f; TargetCurl = 4.f; TargetEars = 18.f;
			TargetRoll = 1.5f * FMath::Sin(Clock * 28.f);
		}
		break;
	}
	case EEldoriaDeityState::Charging:
		TargetPitch = -2.f; TargetHead = -13.f; TargetCurl = 5.f; TargetEars = -8.f;
		break;
	case EEldoriaDeityState::Stunned:
		if (IsHeavilyStunned())
		{
			// Collapsed to its knees, head and trunk on the ground
			TargetZ = -HeavyKneelDepth; TargetPitch = -4.f; TargetRoll = 3.f; TargetHead = -40.f; TargetCurl = 0.f; TargetEars = -18.f; TargetKneel = 80.f;
		}
		else
		{
			TargetZ = -KneelDepth; TargetPitch = -7.f; TargetRoll = 5.f; TargetHead = -22.f; TargetCurl = 1.f; TargetEars = -12.f; TargetKneel = 55.f;
		}
		break;
	case EEldoriaDeityState::Recovering:
		TargetHead = -6.f; TargetCurl = 6.f;
		break;
	case EEldoriaDeityState::Alerted:
		TargetHead = 5.f; TargetCurl = 12.f; TargetEars = 22.f;
		break;
	case EEldoriaDeityState::Liberated:
		if (StateTime < 6.f)
		{
			// Freed: a long, slow bow
			TargetZ = -DrinkCrouch; TargetPitch = -6.f; TargetHead = -25.f; TargetCurl = 2.f; TargetEars = 2.f; TargetKneel = 35.f;
		}
		else
		{
			TargetHead = 6.f + 2.f * FMath::Sin(Clock * 0.5f); TargetCurl = 14.f + 4.f * FMath::Sin(Clock * 0.3f); TargetEars = 8.f;
		}
		break;
	default:
		if (bHeadLowered)
		{
			// Drinking: crouches on its front legs, which brings its back down to the aqueduct's level
			TargetZ = -DrinkCrouch; TargetHead = -24.f; TargetPitch = -4.f; TargetCurl = 2.f; TargetKneel = 25.f;
		}
		break;
	}

	const float Rate = State == EEldoriaDeityState::Charging || State == EEldoriaDeityState::PreparingCharge ? 6.f : 2.5f;
	BodyZ = FMath::FInterpTo(BodyZ, TargetZ, DeltaSeconds, State == EEldoriaDeityState::Stunned ? 2.f : 1.5f);
	// Solid only once the body has actually sunk, and gone as soon as it gets up
	SetNeckRampActive(IsHeavilyStunned() && BodyZ < -HeavyKneelDepth * 0.8f);
	BodyPitch = FMath::FInterpTo(BodyPitch, TargetPitch, DeltaSeconds, Rate);
	BodyRoll = FMath::FInterpTo(BodyRoll, TargetRoll, DeltaSeconds, Rate);
	HeadPitch = FMath::FInterpTo(HeadPitch, TargetHead, DeltaSeconds, Rate);
	TrunkCurl = FMath::FInterpTo(TrunkCurl, TargetCurl, DeltaSeconds, Rate * 0.7f);
	EarFlap = FMath::FInterpTo(EarFlap, TargetEars, DeltaSeconds, 3.f);
	FrontKneel = FMath::FInterpTo(FrontKneel, TargetKneel, DeltaSeconds, 2.f);

	// Body: bob with the gait
	const float Bob = FMath::Sin(GaitPhase * 2.f) * 14.f * FMath::Min(Moving, 1.f);
	BodyRoot->SetRelativeLocationAndRotation(FVector(0.f, 0.f, BodyZ + Bob), FRotator(BodyPitch, 0.f, BodyRoll + FMath::Sin(GaitPhase) * 1.2f * FMath::Min(Moving, 1.f)));

	// Legs: swing with the gait, front legs fold when kneeling, one front foot paws during the telegraph
	const float Swing = LegSwing * FMath::Min(Moving, 1.4f);
	for (int32 Index = 0; Index < LegPivots.Num(); ++Index)
	{
		float Pitch = Swing * FMath::Sin(GaitPhase + LegPhaseOffsets[Index]);
		if (Index < 2)
		{
			Pitch += FrontKneel;
		}
		if (Index == 1)
		{
			Pitch += Paw;
		}
		LegPivots[Index]->SetRelativeRotation(FRotator(Pitch, 0.f, 0.f));
	}

	HeadPivot->SetRelativeRotation(FRotator(HeadPitch + FMath::Sin(GaitPhase * 2.f) * 1.5f * FMath::Min(Moving, 1.f), 0.f, 0.f));

	// Trunk: each segment adds curl plus a travelling sway wave
	for (int32 Index = 0; Index < TrunkPivots.Num(); ++Index)
	{
		const float Weight = (Index + 1) / float(TrunkPivots.Num());
		const float Sway = FMath::Sin(Clock * 1.6f - Index * 0.6f) * 7.f * Weight;
		TrunkPivots[Index]->SetRelativeRotation(FRotator(TrunkCurl + Sway, FMath::Sin(Clock * 1.1f - Index * 0.5f) * 5.f * Weight, 0.f));
	}

	for (int32 Side = 0; Side < EarPivots.Num(); ++Side)
	{
		const float Sign = Side == 0 ? -1.f : 1.f;
		const float Flap = EarFlap + FMath::Sin(Clock * 1.3f + Side) * 5.f;
		EarPivots[Side]->SetRelativeRotation(FRotator(0.f, -Sign * Flap, 0.f));
	}
}
