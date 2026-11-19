// Project Eldoria

#pragma once

#include "CoreMinimal.h"
#include "Interactions/EldoriaInteractableActor.h"
#include "EldoriaControlDevice.generated.h"

class UStaticMeshComponent;
class UMaterialInterface;
class UEldoriaInteractableComponent;
class AEldoriaControlDevice;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FEldoriaDeviceLockReleased, AEldoriaControlDevice*, Device, int32, Released, int32, Total);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FEldoriaDeviceDisabled, AEldoriaControlDevice*, Device);

/**
 * The ancient control mechanism clamped onto a deity's back: an external technological structure
 * (no magic, no gore).
 *
 * To disable it the player must understand how it holds on: three locks on three different sides
 * (walk around it on the creature's back), each one shaking the deity when it pops off; only then is
 * the core exposed and can be pulled out. The device then falls off and the deity is freed.
 */
UCLASS(Blueprintable)
class AEldoriaControlDevice : public AEldoriaInteractableActor
{
	GENERATED_BODY()

public:

	AEldoriaControlDevice();

	/** The casing can be examined only while it is still locked: afterwards E must reach the core, not the casing. */
	virtual bool CanInteract_Implementation(AActor* Interactor) const override;

	/** 0 = intact control, 1 = disabled. Displayed as the boss status. */
	UFUNCTION(BlueprintPure, Category="Device")
	float GetInstability() const;

	UFUNCTION(BlueprintPure, Category="Device")
	bool IsDisabled() const { return bDisabled; }

	/** Releases the next lock, or pulls the core once every lock is gone (debug / scripted use). */
	UFUNCTION(BlueprintCallable, Category="Device")
	void AdvanceSequence(AActor* InstigatorActor);

	UPROPERTY(BlueprintAssignable, Category="Device")
	FEldoriaDeviceLockReleased OnLockReleased;

	UPROPERTY(BlueprintAssignable, Category="Device")
	FEldoriaDeviceDisabled OnDeviceDisabled;

protected:

	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION()
	void HandleLockInteracted(UEldoriaInteractableComponent* Component, AActor* Interactor);

	UFUNCTION()
	void HandleCoreInteracted(UEldoriaInteractableComponent* Component, AActor* Interactor);

	void ReleaseLock(int32 Index, AActor* InstigatorActor);
	void ShutDown(AActor* InstigatorActor);

	UPROPERTY(VisibleAnywhere, Category="Components") TArray<TObjectPtr<UStaticMeshComponent>> Clamps;
	UPROPERTY(VisibleAnywhere, Category="Components") TObjectPtr<USceneComponent> RingPivot;
	UPROPERTY(VisibleAnywhere, Category="Components") TArray<TObjectPtr<UStaticMeshComponent>> Rings;
	UPROPERTY(VisibleAnywhere, Category="Components") TObjectPtr<UEldoriaInteractableComponent> Core;
	UPROPERTY(VisibleAnywhere, Category="Components") TArray<TObjectPtr<UEldoriaInteractableComponent>> Locks;

	UPROPERTY(EditAnywhere, Category="Device|Look") TObjectPtr<UMaterialInterface> MetalMaterial;
	UPROPERTY(EditAnywhere, Category="Device|Look") TObjectPtr<UMaterialInterface> CoreMaterial;
	UPROPERTY(EditAnywhere, Category="Device|Look") TObjectPtr<UMaterialInterface> LockMaterial;

	/** Mechanical rotation of the rings (deg/s); slows down as locks are released. */
	UPROPERTY(EditAnywhere, Category="Device", meta=(Units="deg"))
	float RingSpeed = 25.f;

	/** How long the deity staggers when a lock pops off. */
	UPROPERTY(EditAnywhere, Category="Device", meta=(Units="s"))
	float LockStaggerDuration = 4.f;

private:

	int32 LocksReleased = 0;
	bool bDisabled = false;
};
