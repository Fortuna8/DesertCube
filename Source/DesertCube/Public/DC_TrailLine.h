#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DC_TrailLine.generated.h"

class ADC_Pawn;

UCLASS()
class DESERTCUBE_API ADC_TrailLine : public AActor
{
	GENERATED_BODY()
	
public:	
	ADC_TrailLine();

protected:
	virtual void BeginPlay() override;

public:	
	UPROPERTY(Replicated)
	TObjectPtr<ADC_Pawn> TargetPawn;

	UPROPERTY(ReplicatedUsing = OnRep_InitialPoint)
	FVector InitialPoint;

	UFUNCTION()
	void OnRep_InitialPoint();

	bool bIsInitialized = false;

	// La memoria local de puntos matemáticos
	UPROPERTY()
	TArray<FVector> TurnCorners;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// El RPC Multicast para sincronizar curvas
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_AddTurnPoint(FVector NewPoint);

	// NUEVO: El Escáner Matemático de Colisión
	bool CheckMathematicalCollision(FVector MoveStart, FVector MoveEnd, float BikeRadius, AActor* CheckingPawn);
	
	// Nueva función optimizada para reemplazar al Tick
	void UpdateTrail();

	// El controlador del temporizador
	FTimerHandle TrailUpdateTimer;
};