#include "DC_GameMode.h"
#include "DC_GameState.h"
#include "DC_PlayerState.h"

ADC_GameMode::ADC_GameMode()
{
	// En BP_DC_GameMode las clases vienen por defecto
	GameStateClass = ADC_GameState::StaticClass();
	PlayerStateClass = ADC_PlayerState::StaticClass();
}

void ADC_GameMode::PlayerDied(AController* VictimController)
{
	if (VictimController)
	{
		// Obtenemos el PlayerState de la víctima (el player que chocó)
		ADC_PlayerState* VictimPS = VictimController->GetPlayerState<ADC_PlayerState>();
		if (VictimPS)
		{
			// Lo marcamos como muerto
			VictimPS->bIsAlive = false;
			
			// Log en el servidor para debuggear
			UE_LOG(LogTemp, Warning, TEXT("El jugador %s ha chocado y perdido."), *VictimPS->GetPlayerName());
		}
	}
}