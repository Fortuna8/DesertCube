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

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};