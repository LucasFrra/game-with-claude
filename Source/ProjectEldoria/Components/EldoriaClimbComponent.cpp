// Project Eldoria

#include "Components/EldoriaClimbComponent.h"
#include "Components/EldoriaVitalsComponent.h"
#include "Bosses/EldoriaDeityBase.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"
#include "ProjectEldoria.h"

namespace
{
	const FName NoClimbTag(TEXT("NoClimb"));
	constexpr float ProbeRadius = 12.f;
	constexpr float MantleDuration = 0.45f;
}

UEldoriaClimbComponent::UEldoriaClimbComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

void UEldoriaClimbComponent::BeginPlay()
{
	Super::BeginPlay();
	Character = Cast<ACharacter>(GetOwner());
	Vitals = GetOwner()->FindComponentByClass<UEldoriaVitalsComponent>();
}

bool UEldoriaClimbComponent::FindWall(const FVector& From, const FVector& Direction, float Distance, FHitResult& OutHit) const
{
	FCollisionQueryParams Params(SCENE_QUERY_STAT(EldoriaClimb), false, GetOwner());
	return GetWorld()->SweepSingleByChannel(OutHit, From, From + Direction * Distance, FQuat::Identity, ECC_Pawn,
		FCollisionShape::MakeSphere(ProbeRadius), Params) && !OutHit.bStartPenetrating;
}

bool UEldoriaClimbComponent::IsClimbable(const FHitResult& Hit) const
{
	const UPrimitiveComponent* Component = Hit.GetComponent();
	const AActor* Actor = Hit.GetActor();
	if (!Component || !Actor)
	{
		return false;
	}
	const float NZ = Hit.ImpactNormal.Z;
	if (NZ > MaxClimbNormalZ || NZ < MinClimbNormalZ)
	{
		return false;
	}
	if (Component->IsSimulatingPhysics() || Actor->IsA<APawn>() || Actor->IsA<AEldoriaDeityBase>())
	{
		return false;
	}
	return !Actor->ActorHasTag(NoClimbTag) && !Component->ComponentHasTag(NoClimbTag);
}

bool UEldoriaClimbComponent::TryStart(const FVector& Direction)
{
	const FVector Dir = Direction.GetSafeNormal2D();
	if (Dir.IsNearlyZero() || !Character || (Vitals && Vitals->IsExhausted()))
	{
		return false;
	}
	const UCapsuleComponent* Capsule = Character->GetCapsuleComponent();
	const float R = Capsule->GetScaledCapsuleRadius();
	const float H = Capsule->GetScaledCapsuleHalfHeight();
	const FVector Loc = Character->GetActorLocation();

	FHitResult Chest, Head;
	const float Reach = R + 45.f;
	if (!FindWall(Loc + FVector(0.f, 0.f, 20.f), Dir, Reach, Chest) || !IsClimbable(Chest))
	{
		return false;
	}
	if (FVector::DotProduct(Dir, -Chest.ImpactNormal.GetSafeNormal2D()) < 0.6f)
	{
		return false;		// grazing the wall, not walking into it
	}
	// Only real walls: a step or a rock below the head is walked over, not climbed
	if (!FindWall(Loc + FVector(0.f, 0.f, H - 15.f), Dir, Reach + 20.f, Head) || !IsClimbable(Head))
	{
		return false;
	}
	StartClimbing(Chest);
	return true;
}

void UEldoriaClimbComponent::StartClimbing(const FHitResult& Hit)
{
	UCharacterMovementComponent* Move = Character->GetCharacterMovement();
	bClimbing = true;
	bMantling = false;
	LeapTime = 0.f;
	WallNormal = Hit.ImpactNormal;
	Move->SetMovementMode(MOVE_Flying);
	Move->Velocity = FVector::ZeroVector;
	Move->bOrientRotationToMovement = false;
	Character->SetActorRotation(FRotator(0.f, (-WallNormal).Rotation().Yaw, 0.f));
	UE_LOG(LogProjectEldoria, Log, TEXT("[Climb] grab %s"), *GetNameSafe(Hit.GetActor()));
	OnClimbStateChanged.Broadcast(true);
}

void UEldoriaClimbComponent::StopClimbing()
{
	if (!bClimbing && !bMantling)
	{
		return;
	}
	bClimbing = false;
	bMantling = false;
	RegrabBlock = 0.4f;
	UCharacterMovementComponent* Move = Character->GetCharacterMovement();
	Move->bOrientRotationToMovement = true;
	if (Move->MovementMode == MOVE_Flying)
	{
		Move->SetMovementMode(MOVE_Falling);
	}
	OnClimbStateChanged.Broadcast(false);
}

void UEldoriaClimbComponent::LetGo()
{
	if (bClimbing && !bMantling)
	{
		StopClimbing();
	}
}

bool UEldoriaClimbComponent::HandleJump()
{
	if (!Character)
	{
		return false;
	}
	if (bMantling)
	{
		return true;
	}
	if (bClimbing)
	{
		if (ClimbInput.Y < -0.5f)
		{
			// kick away from the wall
			const FVector Away = WallNormal.GetSafeNormal2D();
			StopClimbing();
			Character->LaunchCharacter(Away * 450.f + FVector(0.f, 0.f, 380.f), true, true);
			return true;
		}
		if (Vitals && !Vitals->ConsumeStamina(StaminaPerLeap))
		{
			StopClimbing();
			return true;
		}
		LeapTime = 0.3f;
		return true;
	}
	// in the air: jumping at a wall grabs it
	if (RegrabBlock <= 0.f && Character->GetCharacterMovement()->IsFalling())
	{
		return TryStart(Character->GetActorForwardVector());
	}
	return false;
}

bool UEldoriaClimbComponent::TryMantle()
{
	const UCapsuleComponent* Capsule = Character->GetCapsuleComponent();
	const float R = Capsule->GetScaledCapsuleRadius();
	const float H = Capsule->GetScaledCapsuleHalfHeight();
	const FVector Loc = Character->GetActorLocation();
	const FVector Into = -WallNormal.GetSafeNormal2D();

	FCollisionQueryParams Params(SCENE_QUERY_STAT(EldoriaMantle), false, GetOwner());
	const FVector Above = Loc + Into * (R + 40.f) + FVector(0.f, 0.f, H + 70.f);
	FHitResult Floor;
	if (!GetWorld()->LineTraceSingleByChannel(Floor, Above, Above - FVector(0.f, 0.f, 2.f * H + 70.f), ECC_Pawn, Params)
		|| Floor.ImpactNormal.Z < 0.65f || Floor.bStartPenetrating)
	{
		return false;
	}
	const FVector End = Floor.ImpactPoint + FVector(0.f, 0.f, H + 3.f);
	// room for the capsule on the ledge
	if (GetWorld()->OverlapBlockingTestByChannel(End, FQuat::Identity, ECC_Pawn, FCollisionShape::MakeCapsule(R, H), Params))
	{
		return false;
	}
	MantleStart = Loc;
	MantleEnd = End;
	MantleAlpha = 0.f;
	bMantling = true;
	Character->GetCharacterMovement()->Velocity = FVector::ZeroVector;
	UE_LOG(LogProjectEldoria, Log, TEXT("[Climb] mantle onto %s"), *GetNameSafe(Floor.GetActor()));
	return true;
}

void UEldoriaClimbComponent::UpdateClimbing(float DeltaTime)
{
	UCharacterMovementComponent* Move = Character->GetCharacterMovement();
	const UCapsuleComponent* Capsule = Character->GetCapsuleComponent();
	const float R = Capsule->GetScaledCapsuleRadius();
	const float H = Capsule->GetScaledCapsuleHalfHeight();
	const FVector Loc = Character->GetActorLocation();
	const FVector2D In(FMath::Clamp(ClimbInput.X, -1.f, 1.f), FMath::Clamp(ClimbInput.Y, -1.f, 1.f));
	const bool bMoving = !In.IsNearlyZero(0.1f) || LeapTime > 0.f;

	if (Vitals && !Vitals->ConsumeStamina((bMoving ? StaminaPerSecondMoving : StaminaPerSecondHolding) * DeltaTime))
	{
		UE_LOG(LogProjectEldoria, Log, TEXT("[Climb] out of stamina"));
		StopClimbing();
		return;
	}

	// Stick to the surface
	FHitResult Hit;
	if (!FindWall(Loc + FVector(0.f, 0.f, 20.f), -WallNormal, R + 60.f, Hit) || !IsClimbable(Hit))
	{
		if (!TryMantle())
		{
			StopClimbing();
		}
		return;
	}
	WallNormal = (WallNormal + (Hit.ImpactNormal - WallNormal) * FMath::Min(1.f, DeltaTime * 10.f)).GetSafeNormal();
	const FVector Up = (FVector::UpVector - WallNormal * FVector::DotProduct(FVector::UpVector, WallNormal)).GetSafeNormal();
	const FVector Right = FVector::CrossProduct(WallNormal, Up);

	FVector Desired = (Up * In.Y + Right * In.X) * ClimbSpeed;
	if (LeapTime > 0.f)
	{
		LeapTime -= DeltaTime;
		Desired = Up * 650.f + Right * In.X * ClimbSpeed;
	}
	const float SurfaceDistance = Hit.Distance + ProbeRadius;
	const float Correction = FMath::Clamp((SurfaceDistance - (R + 4.f)) / FMath::Max(DeltaTime, 0.001f) * 0.5f, -300.f, 300.f);
	Move->Velocity = Desired - WallNormal * Correction;
	Character->SetActorRotation(FRotator(0.f, (-WallNormal).Rotation().Yaw, 0.f));
	SpeedRatio = Desired.Size() / ClimbSpeed;

	// Top of the wall: nothing at head height any more -> pull up
	if (In.Y > 0.1f || LeapTime > 0.f)
	{
		FHitResult HeadHit;
		if (!FindWall(Loc + FVector(0.f, 0.f, H - 5.f), -WallNormal, R + 50.f, HeadHit) || !IsClimbable(HeadHit))
		{
			if (TryMantle())
			{
				return;
			}
		}
	}
	// Bottom: feet on walkable ground while climbing down -> stand
	if (In.Y < -0.1f)
	{
		FCollisionQueryParams Params(SCENE_QUERY_STAT(EldoriaClimbFloor), false, GetOwner());
		FHitResult Floor;
		const FVector Feet = Loc - FVector(0.f, 0.f, H - 5.f);
		if (GetWorld()->LineTraceSingleByChannel(Floor, Feet, Feet - FVector(0.f, 0.f, 30.f), ECC_Pawn, Params) && Floor.ImpactNormal.Z > 0.7f)
		{
			StopClimbing();
			Move->SetMovementMode(MOVE_Walking);
		}
	}
}

void UEldoriaClimbComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!Character)
	{
		return;
	}
	RegrabBlock -= DeltaTime;
	UCharacterMovementComponent* Move = Character->GetCharacterMovement();

	if (bMantling)
	{
		MantleAlpha = FMath::Min(1.f, MantleAlpha + DeltaTime / MantleDuration);
		// up first, then over the edge
		const FVector Raised(MantleStart.X, MantleStart.Y, MantleEnd.Z);
		const FVector P = MantleAlpha < 0.55f
			? FMath::Lerp(MantleStart, Raised, FMath::InterpEaseOut(0.f, 1.f, MantleAlpha / 0.55f, 2.f))
			: FMath::Lerp(Raised, MantleEnd, FMath::InterpEaseInOut(0.f, 1.f, (MantleAlpha - 0.55f) / 0.45f, 2.f));
		Character->SetActorLocation(P, false, nullptr, ETeleportType::TeleportPhysics);
		Move->Velocity = FVector::ZeroVector;
		if (MantleAlpha >= 1.f)
		{
			bMantling = false;
			bClimbing = false;
			Move->bOrientRotationToMovement = true;
			Move->SetMovementMode(MOVE_Walking);
			OnClimbStateChanged.Broadcast(false);
		}
	}
	else if (bClimbing)
	{
		UpdateClimbing(DeltaTime);
	}
	else if (RegrabBlock <= 0.f && !MoveIntent.IsNearlyZero() && (Move->IsMovingOnGround() || Move->IsFalling()))
	{
		// walking into a wall for a moment (or drifting into it in the air) grabs it
		PushingIntoWall += DeltaTime;
		if (PushingIntoWall >= (Move->IsFalling() ? 0.f : 0.18f) && TryStart(MoveIntent))
		{
			PushingIntoWall = 0.f;
		}
	}
	else
	{
		PushingIntoWall = 0.f;
	}
	MoveIntent = FVector::ZeroVector;
	ClimbInput = FVector2D::ZeroVector;
}
