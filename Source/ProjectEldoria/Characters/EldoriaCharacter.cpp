// Project Eldoria

#include "Characters/EldoriaCharacter.h"
#include "Components/EldoriaInteractionComponent.h"
#include "Components/EldoriaPushComponent.h"
#include "Components/EldoriaGrabComponent.h"
#include "Components/EldoriaAbilityComponent.h"
#include "Components/EldoriaVitalsComponent.h"
#include "Components/EldoriaClimbComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "ProjectEldoria.h"
#include "Animation/AnimSequenceBase.h"
#include "UObject/ConstructorHelpers.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Effects/EldoriaJuiceSubsystem.h"
#include "Effects/EldoriaCameraShakes.h"
#include "Bosses/EldoriaDeityBase.h"
#include "Physics/EldoriaPhysicsObject.h"
#include "Camera/CameraComponent.h"
#include "EngineUtils.h"
#include "Kismet/KismetMaterialLibrary.h"
#include "Materials/MaterialParameterCollection.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"

AEldoriaCharacter::AEldoriaCharacter()
{
	InteractionComponent = CreateDefaultSubobject<UEldoriaInteractionComponent>(TEXT("InteractionComponent"));
	PushComponent = CreateDefaultSubobject<UEldoriaPushComponent>(TEXT("PushComponent"));
	GrabComponent = CreateDefaultSubobject<UEldoriaGrabComponent>(TEXT("GrabComponent"));
	AbilityComponent = CreateDefaultSubobject<UEldoriaAbilityComponent>(TEXT("AbilityComponent"));
	VitalsComponent = CreateDefaultSubobject<UEldoriaVitalsComponent>(TEXT("VitalsComponent"));
	ClimbComponent = CreateDefaultSubobject<UEldoriaClimbComponent>(TEXT("ClimbComponent"));
	PrimaryActorTick.bCanEverTick = true;

	// Authored in Blender on the mannequin skeleton (SourceArt/Scripts/blender_anims.py) + template one-shots
	static ConstructorHelpers::FObjectFinder<UAnimSequenceBase> Climb(TEXT("/Game/Project/Characters/Anims/A_Climb.A_Climb"));
	static ConstructorHelpers::FObjectFinder<UAnimSequenceBase> Hang(TEXT("/Game/Project/Characters/Anims/A_ClimbHang.A_ClimbHang"));
	static ConstructorHelpers::FObjectFinder<UAnimSequenceBase> Push(TEXT("/Game/Project/Characters/Anims/A_PushWalk.A_PushWalk"));
	static ConstructorHelpers::FObjectFinder<UAnimSequenceBase> CarryWalk(TEXT("/Game/Project/Characters/Anims/A_CarryWalk.A_CarryWalk"));
	static ConstructorHelpers::FObjectFinder<UAnimSequenceBase> CarryIdle(TEXT("/Game/Project/Characters/Anims/A_CarryIdle.A_CarryIdle"));
	static ConstructorHelpers::FObjectFinder<UAnimSequenceBase> Hit(TEXT("/Game/Characters/Mannequins/Anims/Rifle/HitReact/MM_HitReact_Front_Hvy_01.MM_HitReact_Front_Hvy_01"));
	static ConstructorHelpers::FObjectFinder<UAnimSequenceBase> Throw(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_Attack_01.MM_Attack_01"));
	ClimbAnim = Climb.Object;
	ClimbHangAnim = Hang.Object;
	PushAnim = Push.Object;
	CarryWalkAnim = CarryWalk.Object;
	CarryIdleAnim = CarryIdle.Object;
	HitReactAnim = Hit.Object;
	ThrowAnim = Throw.Object;

	// Adventurer outfit (SourceArt/Scripts/blender_outfit.py)
#define ELDORIA_OUTFIT(Var, Name) static ConstructorHelpers::FObjectFinder<UStaticMesh> Var(TEXT("/Game/Project/Characters/Outfit/" Name "/StaticMeshes/" Name "." Name))
	ELDORIA_OUTFIT(Hood, "SM_Outfit_Hood");
	ELDORIA_OUTFIT(Scarf, "SM_Outfit_Scarf");
	ELDORIA_OUTFIT(Backpack, "SM_Outfit_Backpack");
	ELDORIA_OUTFIT(Satchel, "SM_Outfit_Satchel");
#undef ELDORIA_OUTFIT
	static ConstructorHelpers::FObjectFinder<UMaterialParameterCollection> WorldParams(TEXT("/Game/Project/Materials/MPC_Eldoria.MPC_Eldoria"));
	WorldParameters = WorldParams.Object;
	AddOutfitPiece(TEXT("OutfitHood"), Hood.Object, TEXT("head"));
	AddOutfitPiece(TEXT("OutfitScarf"), Scarf.Object, TEXT("neck_01"));
	AddOutfitPiece(TEXT("OutfitBackpack"), Backpack.Object, TEXT("spine_03"));
	AddOutfitPiece(TEXT("OutfitSatchel"), Satchel.Object, TEXT("pelvis"));

	// Movement: responsive but with a little weight. Values stay editable in the Blueprint.
	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->MaxWalkSpeed = WalkSpeed;
	Move->MaxAcceleration = 2400.f;
	Move->BrakingDecelerationWalking = 2000.f;
	Move->GroundFriction = 8.f;
	Move->RotationRate = FRotator(0.f, 720.f, 0.f);
	Move->JumpZVelocity = 550.f;
	Move->AirControl = 0.45f;
	Move->BrakingDecelerationFalling = 1200.f;
	Move->MaxStepHeight = 45.f;
	Move->SetWalkableFloorAngle(50.f); // natural terrain: slightly steeper slopes than the default 44.7
	Move->MaxFlySpeed = 700.f;          // climbing moves in flying mode (leaps reach 650)

	// Camera: a bit further back for scale, slightly above the shoulder, with soft lag
	if (USpringArmComponent* Boom = GetCameraBoom())
	{
		Boom->TargetArmLength = 450.f;
		Boom->SocketOffset = FVector(0.f, 40.f, 60.f);
		Boom->bEnableCameraLag = true;
		Boom->CameraLagSpeed = 12.f;
		Boom->CameraLagMaxDistance = 100.f;
		Boom->ProbeSize = 14.f;
	}
}

void AEldoriaCharacter::BeginPlay()
{
	Super::BeginPlay();
	RespawnPoint = GetActorTransform();
	FitOutfit();
	if (GetCameraBoom())
	{
		BaseArmLength = GetCameraBoom()->TargetArmLength;
	}
	if (GetFollowCamera())
	{
		BaseFOV = GetFollowCamera()->FieldOfView;
	}
	VitalsComponent->OnDied.AddDynamic(this, &AEldoriaCharacter::HandleDied);
	VitalsComponent->OnStaminaChanged.AddDynamic(this, &AEldoriaCharacter::HandleStaminaChanged);
	VitalsComponent->OnDamaged.AddDynamic(this, &AEldoriaCharacter::HandleDamaged);
	ApplyMovementSpeed();
}

void AEldoriaCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (TestInputTime > 0.f)
	{
		TestInputTime -= DeltaSeconds;
		DoMove(TestInput.X, TestInput.Y);
	}
	UCharacterMovementComponent* Move = GetCharacterMovement();
	if (Move->IsFalling())
	{
		LowestFallVelocityZ = FMath::Min(LowestFallVelocityZ, Move->Velocity.Z);
	}
	UpdateStateAnimation(DeltaSeconds);
	UpdateCameraFeel(DeltaSeconds);

	// sprinting costs breath
	if (bIsSprinting && Move->IsMovingOnGround() && Move->Velocity.Size2D() > 150.f
		&& !VitalsComponent->ConsumeStamina(SprintStaminaPerSecond * DeltaSeconds))
	{
		SetSprinting(false);
	}
}

void AEldoriaCharacter::DoMove(float Right, float Forward)
{
	if (ClimbComponent->IsClimbing())
	{
		ClimbComponent->SetClimbInput(FVector2D(Right, Forward));
		return;
	}
	if (ClimbComponent->IsMantling() || VitalsComponent->IsDead())
	{
		return;
	}
	if (GetController())
	{
		const FRotator Yaw(0.f, GetController()->GetControlRotation().Yaw, 0.f);
		const FVector Intent = FRotationMatrix(Yaw).GetUnitAxis(EAxis::X) * Forward + FRotationMatrix(Yaw).GetUnitAxis(EAxis::Y) * Right;
		ClimbComponent->SetMoveIntent(Intent);
	}
	Super::DoMove(Right, Forward);
}

void AEldoriaCharacter::DoJumpStart()
{
	if (VitalsComponent->IsDead() || ClimbComponent->HandleJump())
	{
		return;
	}
	Super::DoJumpStart();
}

bool AEldoriaCharacter::CanJumpInternal_Implementation() const
{
	if (Super::CanJumpInternal_Implementation())
	{
		return true;
	}
	// coyote time: just walked off a ledge
	const UCharacterMovementComponent* Move = GetCharacterMovement();
	return Move->IsFalling() && !bJumpedSinceGround && LeftGroundTime >= 0.f
		&& GetWorld()->GetTimeSeconds() - LeftGroundTime <= CoyoteTime;
}

void AEldoriaCharacter::OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode)
{
	Super::OnMovementModeChanged(PrevMovementMode, PreviousCustomMode);
	const UCharacterMovementComponent* Move = GetCharacterMovement();
	if (PrevMovementMode == MOVE_Walking && Move->IsFalling())
	{
		LeftGroundTime = GetWorld()->GetTimeSeconds();
		bJumpedSinceGround = Move->Velocity.Z > 100.f;
		LowestFallVelocityZ = 0.f;
	}
	if (Move->MovementMode == MOVE_Flying)
	{
		LowestFallVelocityZ = 0.f;		// climbing resets the fall
	}
}

void AEldoriaCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);
	bJumpedSinceGround = false;
	LeftGroundTime = -1.f;
	const float ImpactSpeed = -LowestFallVelocityZ;
	LowestFallVelocityZ = 0.f;
	if (ImpactSpeed > 900.f)
	{
		if (UEldoriaJuiceSubsystem* Juice = UEldoriaJuiceSubsystem::Get(this))
		{
			const FVector Feet = GetActorLocation() - FVector(0.f, 0.f, GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
			Juice->Dust(Feet, FMath::Clamp(ImpactSpeed / 1800.f, 0.4f, 1.2f), 4);
			Juice->Shake(UEldoriaShakeImpact::StaticClass(), Feet, 0.f, 100.f, FMath::Clamp((ImpactSpeed - 900.f) / 2000.f, 0.1f, 0.6f));
		}
	}
	if (ImpactSpeed > SafeFallSpeed)
	{
		// ~15 m: half a heart, ~25 m: two hearts, beyond ~40 m: fatal
		const int32 Quarters = 2 + FMath::FloorToInt((ImpactSpeed - SafeFallSpeed) / 160.f);
		UE_LOG(LogProjectEldoria, Log, TEXT("[Vitals] hard landing at %.0f cm/s"), ImpactSpeed);
		VitalsComponent->ApplyDamage(Quarters, nullptr);
	}
}

UStaticMeshComponent* AEldoriaCharacter::AddOutfitPiece(FName Name, UStaticMesh* PieceMesh, FName Bone)
{
	UStaticMeshComponent* Piece = CreateDefaultSubobject<UStaticMeshComponent>(Name);
	Piece->SetupAttachment(GetMesh(), Bone);
	Piece->SetStaticMesh(PieceMesh);
	Piece->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Piece->SetCanEverAffectNavigation(false);
	Piece->bReceivesDecals = false;
	OutfitPieces.Add(Piece);
	OutfitBones.Add(Bone);
	return Piece;
}

void AEldoriaCharacter::FitOutfit()
{
	for (int32 Index = 0; Index < OutfitPieces.Num(); ++Index)
	{
		if (UStaticMeshComponent* Piece = OutfitPieces[Index])
		{
			const FVector BoneLocation = GetMesh()->GetSocketLocation(OutfitBones[Index]);
			Piece->SetWorldLocationAndRotation(BoneLocation, GetActorRotation());
		}
	}
}

void AEldoriaCharacter::UpdateStateAnimation(float DeltaSeconds)
{
	USkeletalMeshComponent* MeshComp = GetMesh();
	if (!MeshComp || MeshComp->IsSimulatingPhysics())
	{
		return;		// ragdoll
	}
	const float Speed = GetCharacterMovement()->Velocity.Size2D();
	UAnimSequenceBase* Wanted = nullptr;
	float Rate = 1.f;
	if (OneShotTime > 0.f)
	{
		OneShotTime -= DeltaSeconds;
		return;		// a one-shot (hit, throw) is playing
	}
	if (ClimbComponent->IsMantling())
	{
		Wanted = ClimbAnim;
		Rate = 1.6f;
	}
	else if (ClimbComponent->IsClimbing())
	{
		const float Ratio = ClimbComponent->GetClimbSpeedRatio();
		Wanted = Ratio > 0.1f ? ClimbAnim.Get() : ClimbHangAnim.Get();
		Rate = Ratio > 0.1f ? FMath::Clamp(Ratio, 0.5f, 2.5f) : 1.f;
	}
	else if (PushComponent->IsPushing())
	{
		Wanted = PushAnim;
		Rate = FMath::Clamp(Speed / 160.f, 0.15f, 1.2f);	// a heavy push walks slowly
	}
	else if (GrabComponent->GetCurrentMode() == EEldoriaGrabMode::Carry)
	{
		Wanted = Speed > 30.f ? CarryWalkAnim.Get() : CarryIdleAnim.Get();
		Rate = Speed > 30.f ? FMath::Clamp(Speed / 230.f, 0.6f, 1.6f) : 1.f;
	}

	if (Wanted != CurrentStateAnim)
	{
		if (Wanted)
		{
			MeshComp->PlayAnimation(Wanted, true);
		}
		else
		{
			MeshComp->SetAnimationMode(EAnimationMode::AnimationBlueprint);
		}
		CurrentStateAnim = Wanted;
	}
	if (Wanted)
	{
		MeshComp->SetPlayRate(Rate);
	}
}

void AEldoriaCharacter::PlayOneShot(UAnimSequenceBase* Anim, float Duration)
{
	if (!Anim || GetMesh()->IsSimulatingPhysics())
	{
		return;
	}
	GetMesh()->PlayAnimation(Anim, false);
	GetMesh()->SetPlayRate(1.f);
	OneShotTime = FMath::Min(Duration, Anim->GetPlayLength());
	CurrentStateAnim = Anim;		// forces a switch back once the one-shot ends
}

void AEldoriaCharacter::OnLockOnStarted()
{
	bLockOnHeld = true;
}

void AEldoriaCharacter::OnLockOnStopped()
{
	bLockOnHeld = false;
}

void AEldoriaCharacter::FocusOn(AActor* Target, float Seconds)
{
	FocusTarget = Target;
	FocusTime = Seconds;
}

void AEldoriaCharacter::UpdateCameraFeel(float DeltaSeconds)
{
	UCharacterMovementComponent* Move = GetCharacterMovement();
	const float Speed = Move->Velocity.Size2D();
	const bool bPushing = PushComponent->IsPushing();

	// FOV: wider when sprinting, tighter when straining
	if (UCameraComponent* Cam = GetFollowCamera())
	{
		const float TargetFOV = bPushing ? BaseFOV - 8.f : (bIsSprinting && Speed > 300.f ? BaseFOV + 9.f : BaseFOV);
		Cam->SetFieldOfView(FMath::FInterpTo(Cam->FieldOfView, TargetFOV, DeltaSeconds, 4.f));

		// hurt: colour fringe flash; one heart left: a red, beating vignette
		HurtFlash = FMath::Max(0.f, HurtFlash - DeltaSeconds * 2.f);
		const bool bLow = VitalsComponent->GetHealthQuarters() <= 4 && !VitalsComponent->IsDead();
		const float Pulse = bLow ? 0.5f + 0.5f * FMath::Sin(GetWorld()->GetTimeSeconds() * 5.f) : 0.f;
		FPostProcessSettings& PP = Cam->PostProcessSettings;
		Cam->PostProcessBlendWeight = 1.f;
		PP.bOverride_SceneFringeIntensity = true;
		PP.SceneFringeIntensity = 5.f * HurtFlash;
		PP.bOverride_VignetteIntensity = true;
		PP.VignetteIntensity = 0.22f + (bLow ? 0.35f + 0.25f * Pulse : 0.f) + 0.4f * HurtFlash;
		PP.bOverride_SceneColorTint = true;
		PP.SceneColorTint = FLinearColor::LerpUsingHSV(FLinearColor::White, FLinearColor(1.f, 0.55f, 0.5f), (bLow ? 0.25f * Pulse : 0.f) + 0.3f * HurtFlash);
	}
	if (USpringArmComponent* Boom = GetCameraBoom())
	{
		const float TargetArm = bPushing ? BaseArmLength * 0.75f : (ClimbComponent->IsClimbing() ? BaseArmLength * 1.15f : BaseArmLength);
		Boom->TargetArmLength = FMath::FInterpTo(Boom->TargetArmLength, TargetArm, DeltaSeconds, 3.f);
	}

	// pushing something heavy: a tremor and a puff of dust at its base, every step
	if (bPushing && Speed > 20.f)
	{
		EffortTimer += DeltaSeconds;
		if (EffortTimer > 0.5f)
		{
			EffortTimer = 0.f;
			if (UEldoriaJuiceSubsystem* Juice = UEldoriaJuiceSubsystem::Get(this))
			{
				Juice->Shake(UEldoriaShakeEffort::StaticClass(), GetActorLocation(), 0.f, 100.f, 1.f);
				Juice->Rumble(0.25f, 0.12f);
				if (const AEldoriaPhysicsObject* Pushed = PushComponent->GetPushedObject())
				{
					FVector Origin, Extent;
					Pushed->GetActorBounds(true, Origin, Extent);
					Juice->Dust(FVector(Origin.X, Origin.Y, Origin.Z - Extent.Z + 10.f), 0.6f, 3);
				}
			}
		}
	}

	// Z-targeting / scripted focus
	AActor* Focus = FocusTime > 0.f ? FocusTarget.Get() : nullptr;
	FocusTime -= DeltaSeconds;
	if (!Focus && bLockOnHeld)
	{
		float Best = 9000.f;
		for (TActorIterator<AEldoriaDeityBase> It(GetWorld()); It; ++It)
		{
			const float D = FVector::Dist(It->GetActorLocation(), GetActorLocation());
			if (D < Best)
			{
				Best = D;
				Focus = *It;
			}
		}
	}
	if (Focus && Controller)
	{
		FVector Origin, Extent;
		Focus->GetActorBounds(true, Origin, Extent);
		const FVector Aim = Origin + FVector(0.f, 0.f, Extent.Z * 0.35f);
		const FVector From = GetFollowCamera() ? GetFollowCamera()->GetComponentLocation() : GetActorLocation();
		const FRotator Desired = (Aim - From).Rotation();
		Controller->SetControlRotation(FMath::RInterpTo(Controller->GetControlRotation(), FRotator(FMath::Clamp(Desired.Pitch, -35.f, 25.f), Desired.Yaw, 0.f), DeltaSeconds, 5.f));
	}

	// grass bends away from the player
	if (WorldParameters)
	{
		UKismetMaterialLibrary::SetVectorParameterValue(this, WorldParameters, TEXT("PlayerPosition"), FLinearColor(GetActorLocation() - FVector(0.f, 0.f, GetCapsuleComponent()->GetScaledCapsuleHalfHeight())));
	}
}

void AEldoriaCharacter::HandleDamaged(int32 Quarters)
{
	HurtFlash = 1.f;
	if (UEldoriaJuiceSubsystem* Juice = UEldoriaJuiceSubsystem::Get(this))
	{
		Juice->HitStop(0.08f, 0.03f);
		Juice->Shake(UEldoriaShakeHit::StaticClass(), GetActorLocation(), 0.f, 100.f, FMath::Clamp(Quarters / 4.f, 0.5f, 1.5f));
		Juice->Rumble(0.7f, 0.25f);
	}
	if (!VitalsComponent->IsDead() && !ClimbComponent->IsClimbing())
	{
		PlayOneShot(HitReactAnim, 0.7f);
	}
}

void AEldoriaCharacter::HandleStaminaChanged(float Stamina, bool bExhausted)
{
	if (bExhausted != bWasExhausted)
	{
		bWasExhausted = bExhausted;
		if (bExhausted)
		{
			bIsSprinting = false;
		}
		ApplyMovementSpeed();
	}
}

void AEldoriaCharacter::HandleDied()
{
	ClimbComponent->LetGo();
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		DisableInput(PC);
	}
	// collapse as a ragdoll, then come back at the last safe spot
	GetCharacterMovement()->DisableMovement();
	GetMesh()->SetCollisionProfileName(TEXT("Ragdoll"));
	GetMesh()->SetSimulatePhysics(true);
	GetWorldTimerManager().SetTimer(RespawnTimer, this, &AEldoriaCharacter::Respawn, 3.5f, false);
}

void AEldoriaCharacter::Respawn()
{
	GetWorldTimerManager().ClearTimer(RespawnTimer);
	USkeletalMeshComponent* MeshComp = GetMesh();
	if (MeshComp->IsSimulatingPhysics())
	{
		MeshComp->SetSimulatePhysics(false);
		MeshComp->SetCollisionProfileName(TEXT("CharacterMesh"));
		MeshComp->AttachToComponent(GetCapsuleComponent(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		const ACharacter* Defaults = GetClass()->GetDefaultObject<ACharacter>();
		MeshComp->SetRelativeLocationAndRotation(Defaults->GetMesh()->GetRelativeLocation(), Defaults->GetMesh()->GetRelativeRotation());
	}
	TeleportTo(RespawnPoint.GetLocation(), RespawnPoint.Rotator());
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	VitalsComponent->Revive();
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		EnableInput(PC);
		PC->SetControlRotation(RespawnPoint.Rotator());
	}
	UE_LOG(LogProjectEldoria, Log, TEXT("[Vitals] respawn at %s"), *RespawnPoint.GetLocation().ToCompactString());
}

void AEldoriaCharacter::FellOutOfWorld(const UDamageType& DamageType)
{
	// no game over for a slip: lose a heart and come back
	VitalsComponent->ApplyDamage(4, nullptr);
	if (!VitalsComponent->IsDead())
	{
		Respawn();
	}
}

void AEldoriaCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Template bindings: move, look, jump
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (SprintAction)
		{
			Input->BindAction(SprintAction, ETriggerEvent::Started, this, &AEldoriaCharacter::OnSprintStarted);
			Input->BindAction(SprintAction, ETriggerEvent::Completed, this, &AEldoriaCharacter::OnSprintStopped);
			Input->BindAction(SprintAction, ETriggerEvent::Canceled, this, &AEldoriaCharacter::OnSprintStopped);
		}

		if (ThrowAction)
		{
			Input->BindAction(ThrowAction, ETriggerEvent::Started, this, &AEldoriaCharacter::OnThrow);
		}

		if (AbilityAction)
		{
			Input->BindAction(AbilityAction, ETriggerEvent::Started, this, &AEldoriaCharacter::OnAbility);
		}

		if (LockOnAction)
		{
			Input->BindAction(LockOnAction, ETriggerEvent::Started, this, &AEldoriaCharacter::OnLockOnStarted);
			Input->BindAction(LockOnAction, ETriggerEvent::Completed, this, &AEldoriaCharacter::OnLockOnStopped);
			Input->BindAction(LockOnAction, ETriggerEvent::Canceled, this, &AEldoriaCharacter::OnLockOnStopped);
		}

		if (InteractAction)
		{
			Input->BindAction(InteractAction, ETriggerEvent::Started, this, &AEldoriaCharacter::OnInteractStarted);
			Input->BindAction(InteractAction, ETriggerEvent::Completed, this, &AEldoriaCharacter::OnInteractStopped);
			Input->BindAction(InteractAction, ETriggerEvent::Canceled, this, &AEldoriaCharacter::OnInteractStopped);
		}
	}
}

void AEldoriaCharacter::SetSprinting(bool bNewSprinting)
{
	bIsSprinting = bNewSprinting && !(VitalsComponent && VitalsComponent->IsExhausted());
	ApplyMovementSpeed();
}

void AEldoriaCharacter::SetMovementSpeedCap(float Cap)
{
	SpeedCap = FMath::Max(0.f, Cap);
	ApplyMovementSpeed();
}

void AEldoriaCharacter::ApplyMovementSpeed()
{
	const bool bTired = VitalsComponent && VitalsComponent->IsExhausted();
	const float Speed = bTired ? ExhaustedSpeed : (bIsSprinting ? SprintSpeed : WalkSpeed);
	GetCharacterMovement()->MaxWalkSpeed = SpeedCap > 0.f ? FMath::Min(Speed, SpeedCap) : Speed;
}

void AEldoriaCharacter::OnSprintStarted()
{
	SetSprinting(true);
}

void AEldoriaCharacter::OnSprintStopped()
{
	SetSprinting(false);
}

void AEldoriaCharacter::OnInteractStarted()
{
	if (ClimbComponent->IsClimbing())
	{
		ClimbComponent->LetGo();
		return;
	}
	InteractionComponent->BeginInteract();
}

void AEldoriaCharacter::OnThrow()
{
	const bool bWasCarrying = GrabComponent->GetCurrentMode() == EEldoriaGrabMode::Carry;
	GrabComponent->Throw();
	if (bWasCarrying)
	{
		PlayOneShot(ThrowAnim, 0.55f);
	}
}

void AEldoriaCharacter::OnAbility()
{
	AbilityComponent->ActivatePrimary();
}

void AEldoriaCharacter::OnInteractStopped()
{
	InteractionComponent->EndInteract();
}
