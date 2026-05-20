#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "InputActionValue.h"
#include "DC_Pawn.generated.h"

class UInputMappingContext;
class UInputAction;
class UStaticMeshComponent;
class ADC_TrailSegment;
class UBoxComponent;

UCLASS()
class DESERTCUBE_API ADC_Pawn : public APawn
{
	GENERATED_BODY()

public:
	ADC_Pawn();

protected:
	virtual void BeginPlay() override;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UBoxComponent* CollisionBox;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* MeshComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputMappingContext* DefaultMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* MoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float MovementSpeed;
	
	UPROPERTY(BlueprintReadOnly, Category = "State")
	bool bIsDead;
	
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	float CurrentTargetYaw;

	void Move(const FInputActionValue& Value);
	
	UPROPERTY(EditDefaultsOnly, Category = "Trail")
	TSubclassOf<ADC_TrailSegment> TrailClass;

	UPROPERTY()
	ADC_TrailSegment* CurrentSegment;
	
	FVector LastTurnLocation;

	void SpawnNewSegment();

	UFUNCTION(Server, Reliable)
	void Server_Turn(float NewYaw);
	
	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	// --- OPCIONES DEL MODO SNAKE ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trail Settings")
	bool bIsTrailFinite;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trail Settings", meta = (EditCondition = "bIsTrailFinite", ClampMin = "100.0"))
	float MaxTrailLength;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game Rules")
	bool bDieOnWallCollision;

	// Lista para llevar el registro de todas las paredes vivas
	UPROPERTY()
	TArray<ADC_TrailSegment*> ActiveSegments;
	
public:	
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
};
