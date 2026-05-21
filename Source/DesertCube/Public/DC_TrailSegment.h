#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DC_TrailSegment.generated.h"

class UStaticMeshComponent;
class UBoxComponent;
class USceneComponent;

UCLASS()
class DESERTCUBE_API ADC_TrailSegment : public AActor
{
	GENERATED_BODY()
	
public:	
	ADC_TrailSegment();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USceneComponent* RootScene;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* MeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UBoxComponent* CollisionBox; 

public:	
	// Replicamos las coordenadas para que la red nunca pierda la matemática
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Trail")
	FVector StartLoc;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Trail")
	FVector EndLoc;

	// --- VARIABLES DE ACTOR AUTÓNOMO ---
	UPROPERTY(Replicated)
	bool bIsGrowing;

	UPROPERTY(Replicated)
	AActor* TargetPawn;

	virtual void Tick(float DeltaTime) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	void UpdateSegment(FVector StartLocation, FVector EndLocation);
};