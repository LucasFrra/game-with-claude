// Project Eldoria

#include "Bosses/EldoriaControlDevice.h"
#include "Bosses/EldoriaDeityBase.h"
#include "Interactions/EldoriaInteractableComponent.h"
#include "UI/EldoriaMessageSubsystem.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

#define LOCTEXT_NAMESPACE "EldoriaDevice"

namespace
{
	// The base plate is non-uniformly scaled: child locations are given in cm and converted into its space
	const FVector PlateScale(3.6f, 3.6f, 0.4f);
}

AEldoriaControlDevice::AEldoriaControlDevice()
{
	PrimaryActorTick.bCanEverTick = true;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));

	// Sculpted parts (SourceArt/Scripts/blender_eldoria.py): unit sized like the basic shapes
#define ELDORIA_DEVICE_MESH(Var, Name) static ConstructorHelpers::FObjectFinder<UStaticMesh> Var(TEXT("/Game/Project/Bosses/Tharum/Meshes/" Name "/StaticMeshes/" Name "." Name))
	ELDORIA_DEVICE_MESH(PlateArt, "SM_Device_Plate");
	ELDORIA_DEVICE_MESH(ClampArt, "SM_Device_Clamp");
	ELDORIA_DEVICE_MESH(PillarArt, "SM_Device_Pillar");
	ELDORIA_DEVICE_MESH(RingArt, "SM_Device_Ring");
	ELDORIA_DEVICE_MESH(CoreArt, "SM_Device_Core");
	ELDORIA_DEVICE_MESH(LockArt, "SM_Device_Lock");
#undef ELDORIA_DEVICE_MESH
	auto Pick = [](const auto& Art, const auto& Fallback) { return Art.Succeeded() ? Art.Object : Fallback.Object; };

	Mesh->SetStaticMesh(Pick(PlateArt, Cylinder));
	Mesh->SetRelativeScale3D(PlateScale);
	Mesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));

	auto Place = [this](UStaticMeshComponent* Component, USceneComponent* Parent, UStaticMesh* MeshAsset, const FVector& Offset, const FVector& Scale, const FRotator& Rotation, bool bCollision)
	{
		Component->SetupAttachment(Parent);
		Component->SetStaticMesh(MeshAsset);
		Component->SetRelativeLocation(Parent == Mesh.Get() ? Offset / PlateScale : Offset);
		Component->SetRelativeRotation(Rotation);
		Component->SetUsingAbsoluteScale(true);
		Component->SetRelativeScale3D(Scale);
		if (!bCollision)
		{
			Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
		return Component;
	};

	// Clamps biting into the deity's armor
	for (int32 Index = 0; Index < 4; ++Index)
	{
		const float Angle = Index * 90.f + 45.f;
		const FVector Dir = FRotator(0.f, Angle, 0.f).Vector();
		Clamps.Add(Place(CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("Clamp%d"), Index)), Mesh, Pick(ClampArt, Cube),
			Dir * 165.f + FVector(0.f, 0.f, -20.f), FVector(1.9f, 0.45f, 0.9f), FRotator(-25.f, Angle, 0.f), false));
	}
	Clamps.Add(Place(CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Pillar")), Mesh, Pick(PillarArt, Cylinder), FVector(0.f, 0.f, 90.f), FVector(0.6f, 0.6f, 1.5f), FRotator::ZeroRotator, false));

	RingPivot = CreateDefaultSubobject<USceneComponent>(TEXT("RingPivot"));
	RingPivot->SetupAttachment(Mesh);
	RingPivot->SetRelativeLocation(FVector(0.f, 0.f, 210.f) / PlateScale);	// chest height for a player standing on the back
	RingPivot->SetUsingAbsoluteScale(true);
	Rings.Add(Place(CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RingA")), RingPivot, Pick(RingArt, Cylinder), FVector::ZeroVector, FVector(2.6f, 2.6f, 0.18f), FRotator(0.f, 0.f, 90.f), false));
	Rings.Add(Place(CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RingB")), RingPivot, Pick(RingArt, Cylinder), FVector::ZeroVector, FVector(2.0f, 2.0f, 0.18f), FRotator(90.f, 0.f, 0.f), false));

	Core = CreateDefaultSubobject<UEldoriaInteractableComponent>(TEXT("Core"));
	Place(Core, RingPivot, Pick(CoreArt, Sphere), FVector::ZeroVector, FVector(1.1f, 1.1f, 1.1f), FRotator::ZeroRotator, true);
	Core->Prompt = LOCTEXT("PullCore", "Arracher le noyau");
	Core->bInteractionEnabled = false;

	// Three locks on three sides: the player has to walk around the device to find them all
	for (int32 Index = 0; Index < 3; ++Index)
	{
		const float Angle = Index * 120.f;
		const FVector Dir = FRotator(0.f, Angle, 0.f).Vector();
		UEldoriaInteractableComponent* Lock = CreateDefaultSubobject<UEldoriaInteractableComponent>(*FString::Printf(TEXT("Lock%d"), Index));
		Place(Lock, Mesh, Pick(LockArt, Cube), Dir * 120.f + FVector(0.f, 0.f, 110.f), FVector(0.55f, 0.55f, 0.9f), FRotator(0.f, Angle, 0.f), true);
		Lock->Prompt = LOCTEXT("ReleaseLock", "Desserrer le verrou");
		Locks.Add(Lock);
	}

	InteractionPrompt = LOCTEXT("Examine", "Examiner le mécanisme");
	InspectionText = LOCTEXT("Text", "Un mécanisme d'un autre âge, vissé dans la carapace de Tharum. Trois verrous le maintiennent en place, autour d'un noyau qui palpite. Quelqu'un l'a construit.");
}

bool AEldoriaControlDevice::CanInteract_Implementation(AActor* Interactor) const
{
	return !bDisabled && LocksReleased == 0 && Super::CanInteract_Implementation(Interactor);
}

void AEldoriaControlDevice::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	if (MetalMaterial)
	{
		Mesh->SetMaterial(0, MetalMaterial);
		for (UStaticMeshComponent* Component : Clamps) { Component->SetMaterial(0, MetalMaterial); }
		for (UStaticMeshComponent* Component : Rings) { Component->SetMaterial(0, MetalMaterial); }
	}
	for (UEldoriaInteractableComponent* Lock : Locks)
	{
		if (LockMaterial || CoreMaterial)
		{
			Lock->SetMaterial(0, LockMaterial ? LockMaterial.Get() : CoreMaterial.Get());
		}
		// Sculpted locks: slot 1 is the glowing rune
		if (CoreMaterial && Lock->GetNumMaterials() > 1)
		{
			Lock->SetMaterial(1, CoreMaterial);
		}
	}
	if (CoreMaterial)
	{
		Core->SetMaterial(0, CoreMaterial);
	}
}

void AEldoriaControlDevice::BeginPlay()
{
	Super::BeginPlay();
	for (UEldoriaInteractableComponent* Lock : Locks)
	{
		Lock->OnInteracted.AddDynamic(this, &AEldoriaControlDevice::HandleLockInteracted);
	}
	Core->OnInteracted.AddDynamic(this, &AEldoriaControlDevice::HandleCoreInteracted);
}

float AEldoriaControlDevice::GetInstability() const
{
	return bDisabled ? 1.f : static_cast<float>(LocksReleased) / static_cast<float>(Locks.Num() + 1);
}

void AEldoriaControlDevice::HandleLockInteracted(UEldoriaInteractableComponent* Component, AActor* Interactor)
{
	ReleaseLock(Locks.IndexOfByKey(Component), Interactor);
}

void AEldoriaControlDevice::HandleCoreInteracted(UEldoriaInteractableComponent* Component, AActor* Interactor)
{
	ShutDown(Interactor);
}

void AEldoriaControlDevice::AdvanceSequence(AActor* InstigatorActor)
{
	for (int32 Index = 0; Index < Locks.Num(); ++Index)
	{
		if (Locks[Index]->bInteractionEnabled)
		{
			ReleaseLock(Index, InstigatorActor);
			return;
		}
	}
	ShutDown(InstigatorActor);
}

void AEldoriaControlDevice::ReleaseLock(int32 Index, AActor* InstigatorActor)
{
	if (bDisabled || !Locks.IsValidIndex(Index) || !Locks[Index]->bInteractionEnabled)
	{
		return;
	}

	// The lock pops off and tumbles down the deity's flank
	UEldoriaInteractableComponent* Lock = Locks[Index];
	Lock->bInteractionEnabled = false;
	const FVector Out = (Lock->GetComponentLocation() - Mesh->GetComponentLocation()).GetSafeNormal2D();
	Lock->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
	Lock->SetCollisionProfileName(TEXT("PhysicsActor"));
	Lock->SetSimulatePhysics(true);
	Lock->AddImpulse((Out + FVector(0.f, 0.f, 1.2f)).GetSafeNormal() * 600.f, NAME_None, true);

	++LocksReleased;
	RingSpeed *= 0.55f;
	OnLockReleased.Broadcast(this, LocksReleased, Locks.Num());

	// The deity feels it and staggers: a window to keep going
	if (AEldoriaDeityBase* Deity = Cast<AEldoriaDeityBase>(GetAttachParentActor()))
	{
		Deity->Stun(LockStaggerDuration);
	}

	if (LocksReleased >= Locks.Num())
	{
		Core->bInteractionEnabled = true;
		UEldoriaMessageSubsystem::ShowMessage(this, LOCTEXT("CoreExposed", "Le dernier verrou cède. Le noyau du mécanisme est à nu."), 4.f);
	}
	else
	{
		UEldoriaMessageSubsystem::ShowMessage(this, FText::Format(LOCTEXT("LockReleased", "Un verrou saute ! Encore {0}."), Locks.Num() - LocksReleased), 3.f);
	}
}

void AEldoriaControlDevice::ShutDown(AActor* InstigatorActor)
{
	if (bDisabled || LocksReleased < Locks.Num())
	{
		return;
	}
	bDisabled = true;
	RingSpeed = 0.f;
	Core->bInteractionEnabled = false;
	if (MetalMaterial)
	{
		Core->SetMaterial(0, MetalMaterial);	// the light goes out
	}

	OnDeviceDisabled.Broadcast(this);

	// The dead mechanism slides off the deity's back
	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	Mesh->SetCollisionProfileName(TEXT("PhysicsActor"));
	Mesh->SetSimulatePhysics(true);
	Mesh->SetMassOverrideInKg(NAME_None, 800.f, true);
	Mesh->AddImpulse(FVector(0.f, 350.f, 250.f), NAME_None, true);
}

void AEldoriaControlDevice::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (Rings.Num() == 2 && RingSpeed > 0.f)
	{
		Rings[0]->AddLocalRotation(FRotator(0.f, RingSpeed * DeltaSeconds, 0.f));
		Rings[1]->AddLocalRotation(FRotator(0.f, -RingSpeed * 1.4f * DeltaSeconds, 0.f));
	}
}

#undef LOCTEXT_NAMESPACE
