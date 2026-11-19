// Project Eldoria

#pragma once

#include "CoreMinimal.h"
#include "Bosses/EldoriaDeityBase.h"
#include "EldoriaTharum.generated.h"

class UStaticMeshComponent;
class UCapsuleComponent;
class UBoxComponent;
class USphereComponent;
class UMaterialInterface;
class AEldoriaControlDevice;

/**
 * THARUM - the elephant deity of the jungle.
 *
 * The body is a procedurally animated "puppet" of simple meshes (body, 4 legs, head, segmented trunk,
 * tusks, ears) driven by the deity state and speed: four-beat walking gait, charge telegraph
 * (trunk raised, front foot scraping, then head lowered), stun (kneels, which brings his back
 * within reach), drinking (head and trunk down).
 *
 * Collision is separate from visuals so the visual body can later be replaced by a skeletal mesh
 * without touching gameplay: a capsule body, leg capsules, a head sphere, and a walkable platform
 * on the back where the control device is mounted.
 */
UCLASS(Blueprintable)
class AEldoriaTharum : public AEldoriaDeityBase
{
	GENERATED_BODY()

public:

	AEldoriaTharum();

	UFUNCTION(BlueprintPure, Category="Tharum")
	AEldoriaControlDevice* GetControlDevice() const { return ControlDevice; }

protected:

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void UpdateBodyAnimation(float DeltaSeconds) override;
	// The swept box goes down to the ground (20 cm) so nothing can slip under the charge
	virtual FVector GetChargeSweepExtent() const override { return FVector(280.f, 520.f, 520.f); }
	virtual FVector GetChargeFrontOffset() const override { return FVector(1450.f, 0.f, 540.f); }

	// --- Hierarchy ---
	UPROPERTY(VisibleAnywhere, Category="Components") TObjectPtr<USceneComponent> Root;
	/** Everything that bobs / kneels / leans. */
	UPROPERTY(VisibleAnywhere, Category="Components") TObjectPtr<USceneComponent> BodyRoot;
	UPROPERTY(VisibleAnywhere, Category="Components") TObjectPtr<UStaticMeshComponent> BodyMesh;
	UPROPERTY(VisibleAnywhere, Category="Components") TObjectPtr<UStaticMeshComponent> BellyMesh;
	UPROPERTY(VisibleAnywhere, Category="Components") TObjectPtr<USceneComponent> HeadPivot;
	UPROPERTY(VisibleAnywhere, Category="Components") TObjectPtr<UStaticMeshComponent> HeadMesh;
	UPROPERTY(VisibleAnywhere, Category="Components") TArray<TObjectPtr<USceneComponent>> EarPivots;
	UPROPERTY(VisibleAnywhere, Category="Components") TArray<TObjectPtr<UStaticMeshComponent>> EarMeshes;
	UPROPERTY(VisibleAnywhere, Category="Components") TArray<TObjectPtr<USceneComponent>> TrunkPivots;
	UPROPERTY(VisibleAnywhere, Category="Components") TArray<TObjectPtr<UStaticMeshComponent>> TrunkMeshes;
	UPROPERTY(VisibleAnywhere, Category="Components") TArray<TObjectPtr<UStaticMeshComponent>> TuskMeshes;
	UPROPERTY(VisibleAnywhere, Category="Components") TArray<TObjectPtr<USceneComponent>> LegPivots;
	UPROPERTY(VisibleAnywhere, Category="Components") TArray<TObjectPtr<UStaticMeshComponent>> LegMeshes;
	UPROPERTY(VisibleAnywhere, Category="Components") TArray<TObjectPtr<UStaticMeshComponent>> FootMeshes;
	UPROPERTY(VisibleAnywhere, Category="Components") TArray<TObjectPtr<UStaticMeshComponent>> ArmorMeshes;
	UPROPERTY(VisibleAnywhere, Category="Components") TObjectPtr<UStaticMeshComponent> TailMesh;
	/** Where the control device sits, on top of the back. */
	UPROPERTY(VisibleAnywhere, Category="Components") TObjectPtr<USceneComponent> DeviceSocket;

	// --- Gameplay collision ---
	UPROPERTY(VisibleAnywhere, Category="Collision") TObjectPtr<UCapsuleComponent> BodyCollider;
	UPROPERTY(VisibleAnywhere, Category="Collision") TObjectPtr<UBoxComponent> BackPlatform;
	UPROPERTY(VisibleAnywhere, Category="Collision") TObjectPtr<USphereComponent> HeadCollider;
	/** Walkable slope from the ground in front of the lowered head up to the back; only solid while heavily stunned. */
	UPROPERTY(VisibleAnywhere, Category="Collision") TObjectPtr<UBoxComponent> NeckRamp;
	bool bNeckRampActive = false;
	void SetNeckRampActive(bool bActive);
	UPROPERTY(VisibleAnywhere, Category="Collision") TArray<TObjectPtr<UCapsuleComponent>> LegColliders;

	// --- Look ---
	UPROPERTY(EditAnywhere, Category="Tharum|Look") TObjectPtr<UMaterialInterface> SkinMaterial;
	UPROPERTY(EditAnywhere, Category="Tharum|Look") TObjectPtr<UMaterialInterface> TuskMaterial;
	UPROPERTY(EditAnywhere, Category="Tharum|Look") TObjectPtr<UMaterialInterface> ArmorMaterial;

	UPROPERTY(EditAnywhere, Category="Tharum")
	TSubclassOf<AEldoriaControlDevice> ControlDeviceClass;

	// --- Animation tuning ---
	UPROPERTY(EditAnywhere, Category="Tharum|Animation", meta=(Units="cm"))
	float StrideLength = 520.f;

	UPROPERTY(EditAnywhere, Category="Tharum|Animation", meta=(Units="deg"))
	float LegSwing = 22.f;

	UPROPERTY(EditAnywhere, Category="Tharum|Animation", meta=(Units="cm"))
	float KneelDepth = 420.f;

	/** How much the body lowers while drinking (brings the back to the aqueduct's height). */
	UPROPERTY(EditAnywhere, Category="Tharum|Animation", meta=(Units="cm"))
	float DrinkCrouch = 260.f;

	/** Heavy stun: the body sinks this far, head on the ground; head and neck become a ramp to the back. */
	UPROPERTY(EditAnywhere, Category="Tharum|Animation", meta=(Units="cm"))
	float HeavyKneelDepth = 600.f;

private:

	UStaticMeshComponent* MakeMesh(FName Name, USceneComponent* Parent, UStaticMesh* Mesh, const FVector& Location, const FVector& Scale, const FRotator& Rotation = FRotator::ZeroRotator);

	UPROPERTY(Transient)
	TObjectPtr<AEldoriaControlDevice> ControlDevice;

	float GaitPhase = 0.f;
	float PrevLegCos[4] = { 0.f, 0.f, 0.f, 0.f };
	/** A foot hits the ground: the earth shakes. */
	void OnFootfall(int32 Leg, float Weight);
	float Clock = 0.f;
	float BodyZ = 0.f;
	float BodyPitch = 0.f;
	float BodyRoll = 0.f;
	float HeadPitch = 0.f;
	float TrunkCurl = 0.f;
	float EarFlap = 0.f;
	float FrontKneel = 0.f;
};
