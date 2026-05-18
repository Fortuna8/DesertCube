#include "DC_PlayerState.h"
#include "Net/UnrealNetwork.h"

ADC_PlayerState::ADC_PlayerState()
{
	RoundsWon = 0;
	bIsAlive = true;
}

// Esta función me establece como se replican las variables a los clientes
void ADC_PlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ADC_PlayerState, RoundsWon);
	DOREPLIFETIME(ADC_PlayerState, bIsAlive);
}