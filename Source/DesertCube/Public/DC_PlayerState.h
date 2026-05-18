#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "DC_PlayerState.generated.h"

UCLASS()
class DESERTCUBE_API ADC_PlayerState : public APlayerState
{
	GENERATED_BODY()
	
public:
	ADC_PlayerState();

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "State")
	int32 RoundsWon;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "State")
	bool bIsAlive;

protected:
	// Función obligatoria para registrar las variables replicadas
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};