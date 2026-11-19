// Project Eldoria

#include "Effects/EldoriaDustPuff.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	UStaticMesh* DustMesh = nullptr;
	UMaterialInterface* DustMaterial = nullptr;
}

AEldoriaDustPuff::AEldoriaDustPuff()
{
	PrimaryActorTick.bCanEverTick = true;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Mat(TEXT("/Game/Project/Materials/FX/M_Eldoria_Dust.M_Eldoria_Dust"));
	DustMesh = Sphere.Object;
	DustMaterial = Mat.Object;
	SetActorEnableCollision(false);
}

void AEldoriaDustPuff::Init(float Size, int32 Count)
{
	Life = 1.1f + 0.35f * Size;
	for (int32 Index = 0; Index < Count; ++Index)
	{
		UStaticMeshComponent* Mesh = NewObject<UStaticMeshComponent>(this);
		Mesh->SetStaticMesh(DustMesh);
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Mesh->SetCastShadow(false);
		Mesh->SetupAttachment(GetRootComponent());
		Mesh->RegisterComponent();
		const FVector Dir = FVector(FMath::FRandRange(-1.f, 1.f), FMath::FRandRange(-1.f, 1.f), 0.f).GetSafeNormal();
		Mesh->SetRelativeLocation(Dir * 20.f * Size);
		FPuff Puff;
		Puff.Mesh = Mesh;
		Puff.Material = DustMaterial ? UMaterialInstanceDynamic::Create(DustMaterial, Mesh) : nullptr;
		if (Puff.Material)
		{
			Mesh->SetMaterial(0, Puff.Material);
		}
		// billow outwards along the ground, rise a little
		Puff.Velocity = Dir * FMath::FRandRange(120.f, 260.f) * Size + FVector(0.f, 0.f, FMath::FRandRange(40.f, 110.f) * Size);
		Puff.StartScale = 0.35f * Size * FMath::FRandRange(0.8f, 1.2f);
		Puff.EndScale = 1.5f * Size * FMath::FRandRange(0.8f, 1.25f);
		Puffs.Add(Puff);
	}
}

void AEldoriaDustPuff::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Age += DeltaSeconds;
	const float T = FMath::Clamp(Age / Life, 0.f, 1.f);
	const float Grow = 1.f - FMath::Pow(1.f - T, 3.f);			// fast out, slow end
	for (FPuff& Puff : Puffs)
	{
		if (!Puff.Mesh)
		{
			continue;
		}
		Puff.Velocity *= FMath::Max(0.f, 1.f - 2.2f * DeltaSeconds);	// air drag
		Puff.Mesh->AddRelativeLocation(Puff.Velocity * DeltaSeconds);
		const float S = FMath::Lerp(Puff.StartScale, Puff.EndScale, Grow);
		Puff.Mesh->SetRelativeScale3D(FVector(S, S, S * 0.8f));
		if (Puff.Material)
		{
			Puff.Material->SetScalarParameterValue(TEXT("Opacity"), 0.55f * (1.f - T) * FMath::Min(1.f, T * 8.f));
		}
	}
	if (Age >= Life)
	{
		Destroy();
	}
}
