#include "DC_GameMode.h"
#include "DC_GameState.h"
#include "DC_PlayerState.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"

ADC_GameMode::ADC_GameMode()
{
	GameStateClass = ADC_GameState::StaticClass();
	PlayerStateClass = ADC_PlayerState::StaticClass();
}

void ADC_GameMode::BeginPlay()
{
	Super::BeginPlay();

	// Configuración de las reglas globales del modo Snake en el GameState
	if (ADC_GameState* GS = GetGameState<ADC_GameState>())
	{
		GS->bGlobalIsTrailFinite = true; 
		GS->GlobalMaxTrailLength = 2000.f;
		
		// Temporizador que llama a OnOneSecondPassed cada 1 segundo
		GetWorldTimerManager().SetTimer(RoundTimerHandle, this, &ADC_GameMode::OnOneSecondPassed, 1.0f, true);
	}
}

void ADC_GameMode::OnOneSecondPassed()
{
	if (ADC_GameState* GS = GetGameState<ADC_GameState>())
	{
		if (GS->TimeRemaining > 0)
		{
			GS->TimeRemaining--;

			if (GS->TimeRemaining <= 0)
			{
				UE_LOG(LogTemp, Warning, TEXT("¡Tiempo agotado! Empate."));
				EndRound();
			}
		}
	}
}

void ADC_GameMode::OnPostLogin(AController* NewPlayer)
{
	Super::OnPostLogin(NewPlayer);

	// Cada vez que entra un jugador, actualizamos la cantidad de jugadores vivos
	if (ADC_GameState* GS = GetGameState<ADC_GameState>())
	{
		GS->PlayersAlive = GS->PlayerArray.Num();
		UE_LOG(LogTemp, Warning, TEXT("Jugador conectado. Total de jugadores en arena: %d"), GS->PlayersAlive);
	}
}

void ADC_GameMode::PlayerDied(AController* VictimController)
{
	if (!VictimController) return;

	// 1. Buscamos el PlayerState de la víctima y el GameState general
	ADC_PlayerState* VictimPS = VictimController->GetPlayerState<ADC_PlayerState>();
	ADC_GameState* GS = GetGameState<ADC_GameState>();

	// Verificamos que el jugador estuviera vivo para no procesar la muerte dos veces
	if (VictimPS && VictimPS->bIsAlive)
	{
		VictimPS->bIsAlive = false;
		
		UE_LOG(LogTemp, Warning, TEXT("El jugador %s ha chocado y perdido."), *VictimPS->GetPlayerName());

		// 2. Descontamos un jugador vivo en el tablero público (GameState)
		if (GS)
		{
			GS->PlayersAlive = FMath::Max(0, GS->PlayersAlive - 1);
			UE_LOG(LogTemp, Warning, TEXT("Jugadores restantes en la arena: %d"), GS->PlayersAlive);

			// 3. Resultado de ronda si queda 1 jugador o menos
			if (GS->PlayersAlive <= 1)
			{
				for (APlayerState* PS : GS->PlayerArray)
				{
					ADC_PlayerState* SurvivingPS = Cast<ADC_PlayerState>(PS);
					if (SurvivingPS && SurvivingPS->bIsAlive)
					{
						SurvivingPS->RoundsWon++;
						UE_LOG(LogTemp, Warning, TEXT("¡El jugador %s gana la ronda!"), *SurvivingPS->GetPlayerName());
						break; // Encontramos al ganador
					}
				}
				
				EndRound();
			}
		}
	}
}

void ADC_GameMode::EndRound()
{
	// Frenamos el reloj
	GetWorldTimerManager().ClearTimer(RoundTimerHandle);
	
	// ServerTravel con "?Restart" recarga el mapa actual para todos los jugadores conectados
	if (GetWorld())
	{
		GetWorld()->ServerTravel(TEXT("?Restart"));
	}
}

AActor* ADC_GameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	// 1. Buscamos TODOS los Player Starts que pusiste en el nivel
	TArray<AActor*> FoundStarts;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerStart::StaticClass(), FoundStarts);

	// 2. Si encontramos al menos uno...
	if (FoundStarts.Num() > 0)
	{
		// Usamos el módulo (%) para asegurarnos de que el índice nunca se pase 
		// de la cantidad de Player Starts que existen (Ej: Si hay 2 starts y el índice es 2, vuelve a 0)
		int32 StartToUse = SpawnIndex % FoundStarts.Num();
		
		// Aumentamos el contador para que el próximo jugador use el siguiente
		SpawnIndex++;
		
		UE_LOG(LogTemp, Warning, TEXT("Asignando el Player Start número: %d"), StartToUse);
		
		// Le entregamos este punto de inicio al motor
		return FoundStarts[StartToUse];
	}

	// Si por algún motivo olvidaste poner Player Starts en el mapa, que Unreal haga lo suyo
	return Super::ChoosePlayerStart_Implementation(Player);
}