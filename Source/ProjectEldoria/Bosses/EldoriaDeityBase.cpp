// Project Eldoria

#include "Bosses/EldoriaDeityBase.h"
#include "Environment/EldoriaCollapsibleStructure.h"
#include "Components/EldoriaVitalsComponent.h"
#include "Physics/EldoriaPhysicsObject.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "ProjectEldoria.h"

AEldoriaDeityBase::AEldoriaDeityBase()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AEldoriaDeityBase::BeginPlay()
{
	Super::BeginPlay();
	Player = Cast<ACharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
	SetState(Waypoints.Num() > 0 ? EEldoriaDeityState::Roaming : EEldoriaDeityState::Idle);
}

void AEldoriaDeityBase::SetState(EEldoriaDeityState NewState)
{
	if (State == EEldoriaDeityState::Liberated && NewState != EEldoriaDeityState::Liberated)
	{
		return;		// freedom is permanent
	}

	const EEldoriaDeityState Old = State;
	State = NewState;
	FlankTime = 0.f;
	if (NewState != EEldoriaDeityState::Stunned)
	{
		bHeavyStun = false;
	}
	StateTime = 0.f;
	if (NewState == EEldoriaDeityState::Stunned)
	{
		CurrentStunDuration = StunDuration;
	}
	if (Old != NewState)
	{
		UE_LOG(LogProjectEldoria, Log, TEXT("[Deity] %s: %s -> %s"), *GetName(),
			*StaticEnum<EEldoriaDeityState>()->GetNameStringByValue(static_cast<int64>(Old)),
			*StaticEnum<EEldoriaDeityState>()->GetNameStringByValue(static_cast<int64>(NewState)));
		OnStateChanged.Broadcast(Old, NewState);
	}
}

void AEldoriaDeityBase::Liberate()
{
	SetState(EEldoriaDeityState::Liberated);
}

void AEldoriaDeityBase::Stun(float Duration)
{
	if (State == EEldoriaDeityState::Liberated)
	{
		return;
	}
	if (State == EEldoriaDeityState::Stunned)
	{
		// Keep whichever lasts longer
		CurrentStunDuration = FMath::Max(CurrentStunDuration, StateTime + Duration);
		bHeavyStun |= Duration >= HeavyStunDuration;
		return;
	}
	SetState(EEldoriaDeityState::Stunned);
	CurrentStunDuration = Duration;
	bHeavyStun = Duration >= HeavyStunDuration;
}

bool AEldoriaDeityBase::IsCarrying(const AActor* Actor) const
{
	const ACharacter* Character = Cast<ACharacter>(Actor);
	const UPrimitiveComponent* Base = Character ? Cast<UPrimitiveComponent>(Character->GetMovementBaseObject()) : nullptr;
	return Base && Base->GetOwner() == this;
}

void AEldoriaDeityBase::ReceiveImpact_Implementation(const FEldoriaImpact& Impact)
{
	if (State == EEldoriaDeityState::Liberated || Impact.Instigator == this)
	{
		return;
	}

	// Only something massive can daze a deity: a ruined tower, a falling boulder...
	// Weak structures (young trees, huts) and light objects never can, whatever the contact impulse
	// (contacts against a kinematic body produce huge impulses that do not reflect real momentum).
	if (const AEldoriaCollapsibleStructure* Structure = Cast<AEldoriaCollapsibleStructure>(Impact.Instigator))
	{
		if (Structure->GetImpactThreshold() < 15000.f)
		{
			return;
		}
	}
	if (const AEldoriaPhysicsObject* Object = Cast<AEldoriaPhysicsObject>(Impact.Instigator))
	{
		if (Object->GetMassKg() < 2000.f)
		{
			return;
		}
	}

	if (Impact.Strength >= StunImpactThreshold)
	{
		// A massive structure collapsing onto the deity pins it down much longer
		const bool bHeavy = Cast<AEldoriaCollapsibleStructure>(Impact.Instigator) != nullptr;
		UE_LOG(LogProjectEldoria, Log, TEXT("[Deity] %s stunned by impact %.0f from %s"), *GetName(), Impact.Strength, *GetNameSafe(Impact.Instigator));
		Stun(bHeavy ? HeavyStunDuration : StunDuration);
	}
}

void AEldoriaDeityBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	StateTime += DeltaSeconds;
	UpdatePerception(DeltaSeconds);
	UpdateStateMachine(DeltaSeconds);
	FollowGround();
	UpdateBodyAnimation(DeltaSeconds);

#if ENABLE_DRAW_DEBUG
	if (bDrawDebug)
	{
		const FString Label = StaticEnum<EEldoriaDeityState>()->GetNameStringByValue(static_cast<int64>(State));
		DrawDebugString(GetWorld(), GetActorLocation() + FVector(0.f, 0.f, 1900.f), Label, nullptr, FColor::White, 0.f, true, 1.6f);
	}
#endif
}

// --- Perception -----------------------------------------------------------------------------------

void AEldoriaDeityBase::UpdatePerception(float DeltaSeconds)
{
	TimeSinceSeen += DeltaSeconds;
	PerceptionTimer -= DeltaSeconds;
	if (PerceptionTimer > 0.f)
	{
		return;
	}
	PerceptionTimer = 0.2f;

	if (!Player.IsValid())
	{
		Player = Cast<ACharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
	}
	ACharacter* Target = Player.Get();
	bPlayerVisible = false;
	// A rider on its back is out of sight: the deity goes on with its routine, carrying the player
	if (!Target || State == EEldoriaDeityState::Liberated || IsCarrying(Target))
	{
		return;
	}

	const FVector Eyes = GetActorLocation() + GetActorForwardVector() * 1100.f + FVector(0.f, 0.f, 1100.f);
	const FVector ToPlayer = Target->GetActorLocation() - GetActorLocation();
	const float Distance = ToPlayer.Size2D();

	bool bSees = Distance < HearingRadius;
	if (!bSees && Distance < SightRadius)
	{
		const float Dot = FVector::DotProduct(GetActorForwardVector().GetSafeNormal2D(), ToPlayer.GetSafeNormal2D());
		if (Dot >= FMath::Cos(FMath::DegreesToRadians(SightHalfAngle)))
		{
			FCollisionQueryParams Params(SCENE_QUERY_STAT(DeitySight), false, this);
			Params.AddIgnoredActor(Target);
			bSees = !GetWorld()->LineTraceTestByChannel(Eyes, Target->GetActorLocation() + FVector(0.f, 0.f, 60.f), ECC_Visibility, Params);
		}
	}

	// Outside its domain the player does not concern the deity
	if (!IsInTerritory(Target->GetActorLocation()))
	{
		bSees = false;
	}

	if (bSees != bWasVisible)
	{
		UE_LOG(LogProjectEldoria, Log, TEXT("[Deity] %s %s the player (%.0f m)"), *GetName(), bSees ? TEXT("sees") : TEXT("lost"), Distance / 100.f);
		bWasVisible = bSees;
	}

	if (bSees)
	{
		bPlayerVisible = true;
		TimeSinceSeen = 0.f;
		LastKnownPlayerLocation = Target->GetActorLocation();
	}
}

// --- Movement -------------------------------------------------------------------------------------

bool AEldoriaDeityBase::IsFacing(const FVector& Target, float ToleranceDeg) const
{
	const FVector To = (Target - GetActorLocation()).GetSafeNormal2D();
	return FVector::DotProduct(GetActorForwardVector().GetSafeNormal2D(), To) >= FMath::Cos(FMath::DegreesToRadians(ToleranceDeg));
}

void AEldoriaDeityBase::TurnTowards(const FVector& Target, float DeltaSeconds, float RateMultiplier)
{
	const FVector To = (Target - GetActorLocation()).GetSafeNormal2D();
	if (To.IsNearlyZero())
	{
		return;
	}
	const FRotator Current = GetActorRotation();
	const float Desired = To.Rotation().Yaw;
	const float Delta = FMath::FindDeltaAngleDegrees(Current.Yaw, Desired);
	const float Step = FMath::Clamp(Delta, -TurnRate * RateMultiplier * DeltaSeconds, TurnRate * RateMultiplier * DeltaSeconds);
	SetActorRotation(FRotator(0.f, Current.Yaw + Step, 0.f));
}

void AEldoriaDeityBase::MoveTowards(const FVector& Target, float Speed, float DeltaSeconds, bool bAllowTurnOnly)
{
	TurnTowards(Target, DeltaSeconds);

	// Big bodies turn before they walk
	const bool bFacing = IsFacing(Target, 50.f);
	const float Desired = bFacing ? Speed : (bAllowTurnOnly ? 0.f : Speed * 0.25f);
	CurrentSpeed = FMath::FInterpTo(CurrentSpeed, Desired, DeltaSeconds, 1.5f);
	AddActorWorldOffset(GetActorForwardVector().GetSafeNormal2D() * CurrentSpeed * DeltaSeconds);
}

void AEldoriaDeityBase::FollowGround()
{
	FHitResult Hit;
	const FVector Location = GetActorLocation();
	FCollisionObjectQueryParams Objects(ECC_WorldStatic);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(DeityGround), false, this);
	if (GetWorld()->LineTraceSingleByObjectType(Hit, Location + FVector(0.f, 0.f, 2500.f), Location - FVector(0.f, 0.f, 5000.f), Objects, Params))
	{
		const float TargetZ = Hit.ImpactPoint.Z + GroundOffset;
		SetActorLocation(FVector(Location.X, Location.Y, FMath::FInterpTo(Location.Z, TargetZ, GetWorld()->GetDeltaSeconds(), 6.f)));
	}
}

// --- State machine --------------------------------------------------------------------------------

void AEldoriaDeityBase::UpdateStateMachine(float DeltaSeconds)
{
	ACharacter* Target = Player.Get();
	const float PlayerDistance = Target ? FVector::Dist2D(Target->GetActorLocation(), GetActorLocation()) : TNumericLimits<float>::Max();

	switch (State)
	{
	case EEldoriaDeityState::Idle:
	{
		CurrentSpeed = FMath::FInterpTo(CurrentSpeed, 0.f, DeltaSeconds, 2.f);
		const FEldoriaDeityWaypoint* Point = Waypoints.IsValidIndex(WaypointIndex) ? &Waypoints[WaypointIndex] : nullptr;
		bHeadLowered = Point && Point->bLowerHead;
		if (Point && Point->bUseFacing)
		{
			TurnTowards(GetActorLocation() + FRotator(0.f, Point->FacingYaw, 0.f).Vector() * 1000.f, DeltaSeconds, 0.8f);
		}
		if (bPlayerVisible)
		{
			SetState(EEldoriaDeityState::Alerted);
		}
		else if (Point && StateTime >= Point->WaitTime)
		{
			WaypointIndex = (WaypointIndex + 1) % Waypoints.Num();
			SetState(EEldoriaDeityState::Roaming);
		}
		break;
	}

	case EEldoriaDeityState::Roaming:
	{
		bHeadLowered = false;
		if (bPlayerVisible)
		{
			SetState(EEldoriaDeityState::Alerted);
			break;
		}
		if (!Waypoints.IsValidIndex(WaypointIndex))
		{
			SetState(EEldoriaDeityState::Idle);
			break;
		}
		const FVector Goal = Waypoints[WaypointIndex].Location;
		MoveTowards(Goal, WalkSpeed, DeltaSeconds);
		if (FVector::Dist2D(Goal, GetActorLocation()) < 350.f)
		{
			SetState(EEldoriaDeityState::Idle);
		}
		break;
	}

	case EEldoriaDeityState::Investigating:
		bHeadLowered = false;
		if (bPlayerVisible)
		{
			SetState(EEldoriaDeityState::Alerted);
			break;
		}
		MoveTowards(LastKnownPlayerLocation, WalkSpeed * 1.2f, DeltaSeconds);
		if (FVector::Dist2D(LastKnownPlayerLocation, GetActorLocation()) < 600.f || StateTime > 15.f)
		{
			SetState(EEldoriaDeityState::Roaming);
		}
		break;

	case EEldoriaDeityState::Alerted:
		bHeadLowered = false;
		if (!Target)
		{
			SetState(EEldoriaDeityState::Roaming);
			break;
		}
		if (!bPlayerVisible && TimeSinceSeen > LoseInterestTime)
		{
			SetState(EEldoriaDeityState::Investigating);
			break;
		}
		if (PlayerDistance < ChargeMinDistance)
		{
			// Too close to charge: back away to get some room
			const FVector Away = GetActorLocation() + (GetActorLocation() - Target->GetActorLocation()).GetSafeNormal2D() * 1500.f;
			MoveTowards(Away, WalkSpeed, DeltaSeconds);
		}
		else if (PlayerDistance > ChargeMaxDistance)
		{
			MoveTowards(Target->GetActorLocation(), WalkSpeed, DeltaSeconds);
		}
		else
		{
			CurrentSpeed = FMath::FInterpTo(CurrentSpeed, 0.f, DeltaSeconds, 2.f);
			TurnTowards(Target->GetActorLocation(), DeltaSeconds, 1.2f);
			if (FlankTime > 0.f)
			{
				// Sidestepping an obstacle (a pillar, a rock face) to find a clear lane
				FlankTime -= DeltaSeconds;
				MoveTowards(FlankTarget, WalkSpeed, DeltaSeconds);
				if (FVector::Dist2D(FlankTarget, GetActorLocation()) < 300.f || !IsInTerritory(FlankTarget))
				{
					FlankTime = 0.f;
				}
			}
			else if (StateTime > 1.5f && bPlayerVisible && IsFacing(Target->GetActorLocation(), 12.f))
			{
				if (IsChargeLaneBlocked(Target->GetActorLocation()))
				{
					FlankSide = -FlankSide;
					const FVector Right = FVector::CrossProduct(FVector::UpVector, GetActorForwardVector().GetSafeNormal2D());
					FlankTarget = GetActorLocation() + Right * FlankSide * 1800.f;
					FlankTime = 6.f;
					UE_LOG(LogProjectEldoria, Log, TEXT("[Deity] %s charge lane blocked, flanking"), *GetName());
				}
				else
				{
					SetState(EEldoriaDeityState::PreparingCharge);
				}
			}
		}
		break;

	case EEldoriaDeityState::PreparingCharge:
		CurrentSpeed = FMath::FInterpTo(CurrentSpeed, 0.f, DeltaSeconds, 4.f);
		// Aims during most of the telegraph, then commits: the last moment is the player's window
		if (Target && StateTime < PrepareChargeTime * 0.7f)
		{
			TurnTowards(Target->GetActorLocation(), DeltaSeconds, 1.5f);
		}
		if (StateTime >= PrepareChargeTime)
		{
			ChargeDirection = GetActorForwardVector().GetSafeNormal2D();
			ChargeStart = GetActorLocation();
			ChargeLength = FMath::Clamp((Target ? PlayerDistance : 3000.f) + ChargeOvershoot, 2000.f, 7500.f);
			HitDuringCharge.Reset();
			SetState(EEldoriaDeityState::Charging);
		}
		break;

	case EEldoriaDeityState::Charging:
		UpdateCharge(DeltaSeconds);
		break;

	case EEldoriaDeityState::Recovering:
		CurrentSpeed = FMath::FInterpTo(CurrentSpeed, 0.f, DeltaSeconds, 2.5f);
		AddActorWorldOffset(GetActorForwardVector().GetSafeNormal2D() * CurrentSpeed * DeltaSeconds);
		if (StateTime >= RecoverTime)
		{
			SetState(bPlayerVisible ? EEldoriaDeityState::Alerted : EEldoriaDeityState::Investigating);
		}
		break;

	case EEldoriaDeityState::Stunned:
		CurrentSpeed = 0.f;
		if (StateTime >= CurrentStunDuration)
		{
			SetState(EEldoriaDeityState::Recovering);
		}
		break;

	case EEldoriaDeityState::Liberated:
		CurrentSpeed = FMath::FInterpTo(CurrentSpeed, 0.f, DeltaSeconds, 1.f);
		bHeadLowered = false;
		break;
	}
}

bool AEldoriaDeityBase::IsChargeLaneBlocked(const FVector& Target) const
{
	const FVector Forward = (Target - GetActorLocation()).GetSafeNormal2D();
	const FVector Start = GetActorTransform().TransformPosition(GetChargeFrontOffset());
	const float Length = FMath::Max(FVector::Dist2D(Target, GetActorLocation()) - GetChargeFrontOffset().X, 0.f);
	if (Length <= 0.f)
	{
		return false;
	}
	FCollisionObjectQueryParams Objects(ECC_WorldStatic);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(DeityChargeLane), false, this);
	TArray<FHitResult> Hits;
	GetWorld()->SweepMultiByObjectType(Hits, Start, Start + Forward * Length, Forward.ToOrientationQuat(), Objects, FCollisionShape::MakeBox(GetChargeSweepExtent()), Params);
	for (const FHitResult& Hit : Hits)
	{
		const UPrimitiveComponent* Component = Hit.Component.Get();
		if (!Component || Hit.bStartPenetrating)
		{
			continue;
		}
		if (const AEldoriaCollapsibleStructure* Structure = Cast<AEldoriaCollapsibleStructure>(Hit.GetActor()))
		{
			// Standing structures are fair game, unless they are too close to build up speed
			if (!Structure->IsCollapsed() && Structure->GetImpactThreshold() >= 15000.f && Hit.Distance < 600.f)
			{
				return true;
			}
			continue;
		}
		if (Component->GetMobility() != EComponentMobility::Movable && Hit.ImpactNormal.Z < 0.6f
			&& Component->Bounds.GetBox().Max.Z > GetActorLocation().Z + MinWallHeight)
		{
			return true;
		}
	}
	return false;
}

void AEldoriaDeityBase::UpdateCharge(float DeltaSeconds)
{
	UWorld* World = GetWorld();
	if (!IsInTerritory(GetActorLocation()))
	{
		UE_LOG(LogProjectEldoria, Log, TEXT("[Deity] %s charge ends at the edge of its territory"), *GetName());
		SetState(EEldoriaDeityState::Recovering);
		return;
	}
	CurrentSpeed = FMath::FInterpConstantTo(CurrentSpeed, ChargeSpeed, DeltaSeconds, ChargeSpeed * 1.8f);
	const FVector Delta = ChargeDirection * CurrentSpeed * DeltaSeconds;

	const FVector Front = GetActorTransform().TransformPosition(GetChargeFrontOffset());
	FCollisionObjectQueryParams Objects;
	Objects.AddObjectTypesToQuery(ECC_WorldStatic);
	Objects.AddObjectTypesToQuery(ECC_WorldDynamic);
	Objects.AddObjectTypesToQuery(ECC_PhysicsBody);
	Objects.AddObjectTypesToQuery(ECC_Pawn);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(DeityCharge), false, this);

	TArray<FHitResult> Hits;
	World->SweepMultiByObjectType(Hits, Front, Front + Delta, GetActorQuat(), Objects, FCollisionShape::MakeBox(GetChargeSweepExtent()), Params);

	bool bStopped = false;
	bool bBroughtDown = false;
	for (const FHitResult& Hit : Hits)
	{
		AActor* Other = Hit.GetActor();
		if (!Other || HitDuringCharge.Contains(Other))
		{
			continue;
		}

		if (ACharacter* Character = Cast<ACharacter>(Other))
		{
			HitDuringCharge.Add(Other);
			// Thrown sideways, out of the charge lane, so the deity does not run over the player
			const FVector Right = FVector::CrossProduct(FVector::UpVector, ChargeDirection);
			const float Side = FVector::DotProduct(Character->GetActorLocation() - GetActorLocation(), Right) >= 0.f ? 1.f : -1.f;
			const FVector Push = (Right * Side * 0.85f + ChargeDirection * 0.5f).GetSafeNormal2D() * PlayerKnockbackSpeed + FVector(0.f, 0.f, 900.f);
			Character->LaunchCharacter(Push, true, true);
			if (UEldoriaVitalsComponent* Vitals = Character->FindComponentByClass<UEldoriaVitalsComponent>())
			{
				Vitals->ApplyDamage(ChargeDamageQuarters, this);
			}
			continue;
		}

		if (Other->Implements<UEldoriaImpactReceiver>())
		{
			HitDuringCharge.Add(Other);
			const AEldoriaCollapsibleStructure* Rammed = Cast<AEldoriaCollapsibleStructure>(Other);
			const bool bAlreadyDown = Rammed && Rammed->IsCollapsed();
			FEldoriaImpact Impact;
			Impact.Strength = ChargeImpactStrength * FMath::Clamp(CurrentSpeed / ChargeSpeed, 0.3f, 1.f);
			Impact.Direction = ChargeDirection;
			Impact.Location = Hit.ImpactPoint;
			Impact.Instigator = this;
			IEldoriaImpactReceiver::SendImpact(Other, Impact);
			bBroughtDown |= Rammed && !bAlreadyDown && Rammed->IsCollapsed() && Rammed->GetImpactThreshold() >= 15000.f;

			// Ramming a massive structure hurts: the deity is dazed next to what it broke
			// (what is already down is just rubble: it plows through)
			if (const AEldoriaCollapsibleStructure* Structure = bAlreadyDown ? nullptr : Rammed)
			{
				if (Structure->GetImpactThreshold() >= 15000.f)
				{
					bStopped = true;
					UE_LOG(LogProjectEldoria, Log, TEXT("[Deity] %s charge stopped by structure %s"), *GetName(), *Other->GetName());
				}
			}
			continue;
		}

		// A wall: anything static that faces the charge and is not walkable ground.
		// (Object-type queries do not flag hits as blocking, so bBlockingHit is not checked.)
		const bool bStatic = Hit.Component.IsValid() && Hit.Component->GetMobility() != EComponentMobility::Movable;
		const bool bFacesUs = FVector::DotProduct(Hit.ImpactNormal, -ChargeDirection) > 0.35f;
		// Rocks lower than its knees are simply trampled over: only real walls stop the charge
		const bool bTall = Hit.Component.IsValid() && Hit.Component->Bounds.GetBox().Max.Z > GetActorLocation().Z + MinWallHeight;
		if (bStatic && bTall && bFacesUs && Hit.ImpactNormal.Z < 0.6f && !Hit.bStartPenetrating)
		{
			bStopped = true;
			UE_LOG(LogProjectEldoria, Log, TEXT("[Deity] %s charge stopped by wall %s (%s)"), *GetName(), *Other->GetName(), *GetNameSafe(Hit.Component.Get()));
		}
	}

	if (bStopped)
	{
		// Bringing a whole tower down knocks the deity to its knees for a long time
		if (bBroughtDown)
		{
			Stun(HeavyStunDuration);
		}
		else
		{
			SetState(EEldoriaDeityState::Stunned);
		}
		return;
	}

	AddActorWorldOffset(Delta);
	if (FVector::Dist2D(GetActorLocation(), ChargeStart) >= ChargeLength)
	{
		SetState(EEldoriaDeityState::Recovering);
	}
}
