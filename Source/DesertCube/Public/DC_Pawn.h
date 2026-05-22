#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "InputActionValue.h"
#include "DC_Pawn.generated.h"

class UInputMappingContext;
class UInputAction;
class UStaticMeshComponent;
class UBoxComponent;
class ADC_TrailLine; 

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
	TSubclassOf<ADC_TrailLine> TrailLineClass;

	UPROPERTY(Replicated)
	ADC_TrailLine* MyTrailLine; 

	void InitializeTrail();
	
	UFUNCTION(Server, Reliable)
	void Server_Turn(float NewYaw);

	// Función interna de muerte
	void Die();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trail Settings")
	bool bIsTrailFinite;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trail Settings", meta = (EditCondition = "bIsTrailFinite", ClampMin = "100.0"))
	float MaxTrailLength;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game Rules")
	bool bDieOnWallCollision;

public:	
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_StartRound();
	
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_StopRound();

	UFUNCTION(NetMulticast, Reliable) 
	void Multicast_Die();
	
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};