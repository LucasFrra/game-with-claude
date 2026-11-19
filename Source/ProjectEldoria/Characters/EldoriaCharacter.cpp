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

void AEldoriaCharacter::HandleDamaged(int32 Quarters)
{
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
