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
	// Nueva raíz vacía para poder desplazar los otros componentes
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USceneComponent* RootScene;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* MeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UBoxComponent* CollisionBox; 

public:	
	void UpdateSegment(FVector StartLocation, FVector EndLocation);
};