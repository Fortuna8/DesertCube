#include "DC_GameInstance.h"

void UDC_GameInstance::AddWin(const FString& PlayerName)
{
	// Si el jugador ya existe en la memoria, le sumamos 1. Si no, lo creamos con 1 victoria.
	if (PlayerScores.Contains(PlayerName))
	{
		PlayerScores[PlayerName]++;
	}
	else
	{
		PlayerScores.Add(PlayerName, 1);
	}
}

int32 UDC_GameInstance::GetWins(const FString& PlayerName)
{
	// Devolvemos sus victorias. Si es un jugador nuevo, devolvemos 0.
	if (PlayerScores.Contains(PlayerName))
	{
		return PlayerScores[PlayerName];
	}
	return 0;
}

void UDC_GameInstance::ResetTournament()
{
	// Limpiamos la memoria para cuando alguien gane la partida definitiva
	PlayerScores.Empty();
}