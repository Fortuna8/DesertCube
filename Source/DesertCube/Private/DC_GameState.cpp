#include "DC_GameState.h"
#include "Net/UnrealNetwork.h"

ADC_GameState::ADC_GameState()
{
	PlayersAlive = 0;
}

void ADC_GameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ADC_GameState, PlayersAlive);
}