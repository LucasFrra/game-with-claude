// Project Eldoria

#include "Abilities/EldoriaAbility.h"
#include "Components/EldoriaPushComponent.h"
#include "Interactions/EldoriaImpactReceiver.h"
#include "Physics/EldoriaPhysicsObject.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "ProjectEldoria.h"

#define LOCTEXT_NAMESPACE "EldoriaAbility"

void UEldoriaAbility::OnGranted(ACharacter* Owner)
{
	ReceiveGranted(Owner);
}

bool UEldoriaAbility::Activate(ACharacter* Owner)
{
	const UWorld* World = Owner ? Owner->GetWorld() : nullptr;
	if (!World || World->GetTimeSeconds() - LastActivationTime < Cooldown)
	{
		return false;
	}
	LastActivationTime = World->GetTimeSeconds();
	ReceiveActivated(Owner);
	return true;
}

UEldoriaAbility_Force::UEldoriaAbility_Force()
{
	DisplayName = LOCTEXT("ForceName", "FORCE");
	Description = LOCTEXT("ForceDesc", "La puissance de Tharum coule en vous. Vous pouvez déplacer ce qui était inébranlable. [F] : poussée de FORCE.");
	Cooldown = 0.9f;
}

void UEldoriaAbility_Force::OnGranted(ACharacter* Owner)
{
	if (UEldoriaPushComponent* Push = Owner ? Owner->FindComponentByClass<UEldoriaPushComponent>() : nullptr)
	{
		Push->SetStrength(FMath::Max(Push->GetStrength(), Strength));
	}
	Super::OnGranted(Owner);
}

bool UEldoriaAbility_Force::Activate(ACharacter* Owner)
{
	if (!Super::Activate(Owner))
	{
		return false;
	}

	UWorld* World = Owner->GetWorld();
	const FVector Forward = Owner->GetActorForwardVector().GetSafeNormal2D();
	const FVector Center = Owner->GetActorLocation() + Forward * (ShoveRange * 0.5f);

	FCollisionObjectQueryParams Objects;
	Objects.AddObjectTypesToQuery(ECC_PhysicsBody);
	Objects.AddObjectTypesToQuery(ECC_WorldDynamic);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(EldoriaForceShove), false, Owner);
	TArray<FOverlapResult> Overlaps;
	World->OverlapMultiByObjectType(Overlaps, Center, FQuat::Identity, Objects, FCollisionShape::MakeSphere(ShoveRange * 0.6f), Params);

	bool bHitSomething = false;
	TSet<AActor*> Done;
	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* Actor = Overlap.GetActor();
		if (!Actor || Done.Contains(Actor))
		{
			continue;
		}
		Done.Add(Actor);

		if (AEldoriaPhysicsObject* Object = Cast<AEldoriaPhysicsObject>(Actor))
		{
			// Wakes even frozen heavy objects; the result depends on their mass
			Object->SetBeingHandled(true);
			Object->SetBeingHandled(false);
			float Speed = FMath::Min(ShoveMomentum * 100.f / FMath::Max(Object->GetMassKg(), 1.f), MaxShoveSpeed);
			if (Object->GetRequiredStrength() > 1.f && Object->GetRequiredStrength() <= Strength)
			{
				Speed = FMath::Max(Speed, MinUnlockedShoveSpeed);
			}
			Object->GetMesh()->AddImpulse((Forward + FVector(0.f, 0.f, 0.15f)).GetSafeNormal() * Speed, NAME_None, true);
			UE_LOG(LogProjectEldoria, Log, TEXT("[Ability] FORCE shoves %s (%.0f kg) at %.0f cm/s"), *Object->GetName(), Object->GetMassKg(), Speed);
			bHitSomething = true;
		}
		else if (Actor->Implements<UEldoriaImpactReceiver>())
		{
			FEldoriaImpact Impact;
			Impact.Strength = ShoveMomentum;
			Impact.Direction = Forward;
			Impact.Location = Overlap.GetComponent() ? Overlap.GetComponent()->GetComponentLocation() : Center;
			Impact.Instigator = Owner;
			IEldoriaImpactReceiver::SendImpact(Actor, Impact);
			UE_LOG(LogProjectEldoria, Log, TEXT("[Ability] FORCE impact on %s"), *Actor->GetName());
			bHitSomething = true;
		}
	}
	return bHitSomething;
}

#undef LOCTEXT_NAMESPACE
