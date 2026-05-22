#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DC_TrailLine.generated.h"

class USplineComponent;
class USplineMeshComponent;
class UStaticMesh;
class UMaterialInterface;
class ADC_Pawn;

UCLASS()
class DESERTCUBE_API ADC_TrailLine : public AActor
{
	GENERATED_BODY()
	
public:	
	ADC_TrailLine();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USplineComponent* SplineComp;

	UPROPERTY(EditDefaultsOnly, Category = "Visuals")
	UStaticMesh* TrailMesh;

	UPROPERTY(EditDefaultsOnly, Category = "Visuals")
	UMaterialInterface* TrailMaterial;

public:	
	UPROPERTY(Replicated)
	ADC_Pawn* TargetPawn;

	// El punto garantizado de nacimiento contra el lag
	UPROPERTY(ReplicatedUsing = OnRep_InitialPoint)
	FVector InitialPoint;

	UFUNCTION()
	void OnRep_InitialPoint();

	bool bIsInitialized = false;

	// La memoria local de cada computadora (NO se replica, ahorra toda la red)
	UPROPERTY()
	TArray<FVector> TurnCorners;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void Tick(float DeltaTime) override;
	bool IsSafeSegment(UPrimitiveComponent* Comp);

	// EL REQUISITO DEL PROFE: RPC Multicast garantizado para sincronizar curvas
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_AddTurnPoint(FVector NewPoint);

private:
	void UpdateSplineMeshes(const TArray<FVector>& Points);

	UPROPERTY()
	TArray<USplineMeshComponent*> SplineMeshes;
};