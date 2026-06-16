#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "InputActionValue.h"
#include "NiagaraComponent.h"
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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UNiagaraComponent* TrailNiagaraComponent;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputMappingContext* DefaultMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* MoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float MovementSpeed;
	
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
	
	UPROPERTY(BlueprintReadOnly, Category = "State")
	bool bIsDead;
	
	// 2. La variable de color que viaja por la red
	UPROPERTY(ReplicatedUsing = OnRep_PlayerColorIndex, BlueprintReadOnly, Category = "Identity")
	int32 PlayerColorIndex;
	
	// 3. La función que se dispara en los Clientes cuando reciben el color
	UFUNCTION()
	void OnRep_PlayerColorIndex();
	
	// 4. Se ejecuta en el Servidor cuando la moto es asignada a un jugador
	virtual void PossessedBy(AController* NewController) override;

	// 5. Función auxiliar para inyectar el color en Niagara
	void UpdateTrailColor();
	
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_StartRound();
	
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_StopRound();

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_Die(const FString& VictimName);

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_OnWin(); // <--- Cambiamos Client por NetMulticast
	
	UFUNCTION(BlueprintImplementableEvent, Category = "UI")
	void OnReceiveWinUI();

	UFUNCTION(BlueprintImplementableEvent, Category = "UI")
	void OnReceiveLoseUI();

	UFUNCTION(BlueprintImplementableEvent, Category = "UI")
	void OnReceiveOtherPlayerNoticeUI(const FString& OtherPlayerName);
	
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_MatchOver();

	UFUNCTION(BlueprintImplementableEvent, Category = "UI")
	void OnReceiveScoreboardUI();
};