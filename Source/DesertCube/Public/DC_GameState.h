#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "DC_GameState.generated.h"

UCLASS()
class DESERTCUBE_API ADC_GameState : public AGameStateBase
{
	GENERATED_BODY()
	
public:
	ADC_GameState();

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Game State")
	int32 PlayersAlive;
	
	// En la sección public de DC_GameState.h
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadOnly, Category = "Game Rules")
	bool bGlobalIsTrailFinite;

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadOnly, Category = "Game Rules")
	float GlobalMaxTrailLength;
	
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Game State")
	int32 TimeRemaining;

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};