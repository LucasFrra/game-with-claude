// Project Eldoria

#include "Characters/EldoriaCharacter.h"
#include "Components/EldoriaInteractionComponent.h"
#include "Components/EldoriaPushComponent.h"
#include "Components/EldoriaGrabComponent.h"
#include "Components/EldoriaAbilityComponent.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"

AEldoriaCharacter::AEldoriaCharacter()
{
	InteractionComponent = CreateDefaultSubobject<UEldoriaInteractionComponent>(TEXT("InteractionComponent"));
	PushComponent = CreateDefaultSubobject<UEldoriaPushComponent>(TEXT("PushComponent"));
	GrabComponent = CreateDefaultSubobject<UEldoriaGrabComponent>(TEXT("GrabComponent"));
	AbilityComponent = CreateDefaultSubobject<UEldoriaAbilityComponent>(TEXT("AbilityComponent"));

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
	ApplyMovementSpeed();
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
	bIsSprinting = bNewSprinting;
	ApplyMovementSpeed();
}

void AEldoriaCharacter::SetMovementSpeedCap(float Cap)
{
	SpeedCap = FMath::Max(0.f, Cap);
	ApplyMovementSpeed();
}

void AEldoriaCharacter::ApplyMovementSpeed()
{
	const float Speed = bIsSprinting ? SprintSpeed : WalkSpeed;
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
	InteractionComponent->BeginInteract();
}

void AEldoriaCharacter::OnThrow()
{
	GrabComponent->Throw();
}

void AEldoriaCharacter::OnAbility()
{
	AbilityComponent->ActivatePrimary();
}

void AEldoriaCharacter::OnInteractStopped()
{
	InteractionComponent->EndInteract();
}
