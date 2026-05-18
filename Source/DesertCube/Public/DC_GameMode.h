#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "DC_GameMode.generated.h"

UCLASS()
class DESERTCUBE_API ADC_GameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ADC_GameMode();

	// Función que llamará el Pawn cuando detecte un choque fatal
	UFUNCTION(BlueprintCallable, Category = "Game Rules")
	void PlayerDied(AController* VictimController);
};