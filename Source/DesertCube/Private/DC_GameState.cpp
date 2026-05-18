#include "DC_GameState.h"
#include "Net/UnrealNetwork.h"

ADC_GameState::ADC_GameState()
{
	PlayersAlive = 0;
	bGlobalIsTrailFinite = true;
	GlobalMaxTrailLength = 2000.f;
	
	// Iniciamos la ronda con 60 segundos
	TimeRemaining = 60;
}

void ADC_GameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ADC_GameState, PlayersAlive);
	DOREPLIFETIME(ADC_GameState, bGlobalIsTrailFinite);
	DOREPLIFETIME(ADC_GameState, GlobalMaxTrailLength);
	DOREPLIFETIME(ADC_GameState, TimeRemaining);
}