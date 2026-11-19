// Project Eldoria

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interactions/EldoriaImpactReceiver.h"
#include "EldoriaCollapsibleStructure.generated.h"

UENUM(BlueprintType)
enum class EEldoriaCollapseMode : uint8
{
	/** The solid pieces are welded into one rigid body that is knocked over and falls physically. */
	Topple,
	/** Every piece becomes an independent rigid body thrown by the impact. */
	Shatter
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FEldoriaStructureCollapsed, AEldoriaCollapsibleStructure*, Structure);

/**
 * Weakened structure (ruined tower, young tree, wooden hut...) that collapses when it receives
 * an impact stronger than ImpactThreshold (see FEldoriaImpact).
 *
 * Authoring: add StaticMesh components under "ToppleRoot". Pieces tagged "Loose" break off as
 * separate debris when the structure collapses.
 *
 * The fall is fully simulated by Chaos: the structure is knocked at its top in the impact direction
 * and pivots on its base through ground friction. Where it lands depends on the world (a ledge,
 * a creature...). Once settled it can freeze so the result is a stable ramp to walk on.
 * Whatever it hits hard receives an impact in turn.
 */
UCLASS(Blueprintable)
class AEldoriaCollapsibleStructure : public AActor, public IEldoriaImpactReceiver
{
	GENERATED_BODY()

public:

	AEldoriaCollapsibleStructure();

	// IEldoriaImpactReceiver
	virtual void ReceiveImpact_Implementation(const FEldoriaImpact& Impact) override;

	/** Collapses the structure. Direction is horizontalized. Does nothing if already collapsed. */
	UFUNCTION(BlueprintCallable, Category="Collapse")
	void Collapse(FVector Direction, AActor* InstigatorActor);

	/** Editor / PIE test button: collapses along the actor's forward vector. */
	UFUNCTION(CallInEditor, BlueprintCallable, Category="Collapse")
	void DebugCollapse();

	UFUNCTION(BlueprintPure, Category="Collapse")
	bool IsCollapsed() const { return bCollapsed; }

	UFUNCTION(BlueprintPure, Category="Collapse")
	float GetImpactThreshold() const { return ImpactThreshold; }

	UPROPERTY(BlueprintAssignable, Category="Collapse")
	FEldoriaStructureCollapsed OnCollapsed;

	/** Fired once the fallen structure has come to rest. */
	UPROPERTY(BlueprintAssignable, Category="Collapse")
	FEldoriaStructureCollapsed OnSettled;

protected:

	UFUNCTION()
	void HandleBodyHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	void CheckSettled();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<USceneComponent> Root;

	/** Parent of all the structure's pieces. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<USceneComponent> ToppleRoot;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Collapse")
	EEldoriaCollapseMode CollapseMode = EEldoriaCollapseMode::Topple;

	/** Minimum impact momentum (kg*m/s) needed to bring it down. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Collapse", meta=(ClampMin=0))
	float ImpactThreshold = 20000.f;

	/** If set, the structure always falls this way (its weakened side), whatever the impact direction. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Collapse")
	bool bUseFixedFallDirection = false;

	/** A ram crushes the weakened base on the struck side: the structure tips back onto whatever hit it
	 *  (takes priority over the fixed direction when the impact has an instigator). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Collapse")
	bool bFallTowardInstigator = false;

	/** Local-space fall direction used when bUseFixedFallDirection is true. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Collapse", meta=(EditCondition="bUseFixedFallDirection"))
	FVector FixedFallDirection = FVector::ForwardVector;

	/** Total mass of the welded structure when toppling. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Collapse|Topple", meta=(ClampMin=1, Units="kg"))
	float StructureMassKg = 20000.f;

	/** Speed given to the top of the structure by the impact (it then falls under gravity). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Collapse|Topple", meta=(ClampMin=0, Units="CentimetersPerSecond"))
	float TopKickSpeed = 250.f;

	/** Freeze the fallen structure once it rests, so it becomes stable ground (a ramp, a bridge...). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Collapse|Topple")
	bool bFreezeWhenSettled = true;

	/** Momentum (kg*m/s) above which what the falling structure hits receives an impact. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Collapse|Topple", meta=(ClampMin=0))
	float MinReportedImpact = 5000.f;

	/** Velocity given to debris pieces. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Collapse", meta=(ClampMin=0, Units="CentimetersPerSecond"))
	float DebrisVelocity = 500.f;

	/** Mass given to each debris piece. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Collapse", meta=(ClampMin=1, Units="kg"))
	float DebrisMassKg = 400.f;

private:

	void ReleaseAsDebris(UPrimitiveComponent* Piece, const FVector& Direction);

	bool bCollapsed = false;
	bool bSettled = false;
	int32 RestChecks = 0;
	double LastReportedImpactTime = -100.0;
	FVector FallDirection = FVector::ForwardVector;
	TWeakObjectPtr<AActor> CollapseInstigator;

	UPROPERTY(Transient)
	TObjectPtr<UPrimitiveComponent> MainBody;

	FTimerHandle SettleTimer;
};
