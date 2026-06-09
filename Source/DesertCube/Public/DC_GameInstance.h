#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "DC_GameInstance.generated.h"

UCLASS()
class DESERTCUBE_API UDC_GameInstance : public UGameInstance
{
	GENERATED_BODY()
	
public:
	// El diccionario inborrable: vincula "Nombre de Jugador" -> "Victorias"
	UPROPERTY(BlueprintReadWrite, Category = "Tournament")
	TMap<FString, int32> PlayerScores;

	// Funciones para que el GameMode gestione los puntos
	void AddWin(const FString& PlayerName);
	int32 GetWins(const FString& PlayerName);
	void ResetTournament();
};