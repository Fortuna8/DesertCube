#include "DC_GameMode.h"
#include "DC_GameState.h"
#include "DC_PlayerState.h"
#include "DC_Pawn.h"
#include "DC_GameInstance.h"
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

	if (ADC_GameState* GS = GetGameState<ADC_GameState>())
	{
		GS->bGlobalIsTrailFinite = true; 
		GS->GlobalMaxTrailLength = 2000.f;
	}
	
	// ELIMINAR O COMENTAR ESTO:
	// GetWorldTimerManager().SetTimer(WarmupTimerHandle, this, &ADC_GameMode::OnWarmupTick, 1.0f, true);
}

void ADC_GameMode::OnPostLogin(AController* NewPlayer)
{
	Super::OnPostLogin(NewPlayer);

	if (ADC_PlayerState* PS = NewPlayer->GetPlayerState<ADC_PlayerState>())
	{
		if (UDC_GameInstance* GI = Cast<UDC_GameInstance>(GetGameInstance()))
		{
			// El GameMode le inyecta a la red los puntos que sobrevivieron al reinicio
			PS->RoundsWon = GI->GetWins(PS->GetPlayerName());
		}
	}
	
	if (ADC_GameState* GS = GetGameState<ADC_GameState>())
	{
		GS->PlayersAlive = GS->PlayerArray.Num();
		UE_LOG(LogTemp, Warning, TEXT("Jugador conectado. Total de jugadores en arena: %d"), GS->PlayersAlive);

		// Verificamos si ya tenemos la cantidad mínima de jugadores para arrancar
		int32 JugadoresNecesariosParaEmpezar = 2; // O 4, dependiendo de tu testeo actual

		if (GS->PlayersAlive >= JugadoresNecesariosParaEmpezar)
		{
			// ¡Ahora sí! Como ya llegaron todos, disparamos el calentamiento
			GetWorldTimerManager().SetTimer(WarmupTimerHandle, this, &ADC_GameMode::OnWarmupTick, 1.0f, true);
			
			UE_LOG(LogTemp, Warning, TEXT("¡Todos los jugadores conectados! Iniciando secuencia de 3, 2, 1..."));
		}
	}
}

void ADC_GameMode::OnWarmupTick()
{
	WarmupTime--;

	if (WarmupTime > 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("Comenzando en... %d"), WarmupTime);
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Yellow, FString::Printf(TEXT("Comenzando en... %d"), WarmupTime));
	}
	else
	{
		// Frenamos el contador de calentamiento
		GetWorldTimerManager().ClearTimer(WarmupTimerHandle);
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green, TEXT("¡GO!"));
		
		StartMatch();
	}
}

void ADC_GameMode::StartMatch()
{
	// 1. Iniciamos el reloj oficial de la partida
	GetWorldTimerManager().SetTimer(RoundTimerHandle, this, &ADC_GameMode::OnOneSecondPassed, 1.0f, true);

	// 2. Buscamos a TODOS los jugadores conectados y les damos la orden de acelerar
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (PC && PC->GetPawn())
		{
			if (ADC_Pawn* MyPawn = Cast<ADC_Pawn>(PC->GetPawn()))
			{
				MyPawn->Multicast_StartRound();
			}
		}
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

void ADC_GameMode::PlayerDied(AController* VictimController)
{
	if (!VictimController) return;

	ADC_PlayerState* VictimPS = VictimController->GetPlayerState<ADC_PlayerState>();
	ADC_GameState* GS = GetGameState<ADC_GameState>();

	if (VictimPS && VictimPS->bIsAlive)
	{
		VictimPS->bIsAlive = false;
		UE_LOG(LogTemp, Warning, TEXT("El jugador %s ha chocado y perdido."), *VictimPS->GetPlayerName());

		if (GS)
		{
			GS->PlayersAlive = FMath::Max(0, GS->PlayersAlive - 1);
			UE_LOG(LogTemp, Warning, TEXT("Jugadores restantes en la arena: %d"), GS->PlayersAlive);

			if (GS->PlayersAlive <= 1)
			{
				// --- NUEVA LÓGICA DE DETECCIÓN INFACOBLE ---
				// Buscamos directamente en el mapa qué moto quedó en pie y con bIsDead en falso
				for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
				{
					APlayerController* PC = It->Get();
					if (PC && PC->GetPawn())
					{
						ADC_Pawn* TempPawn = Cast<ADC_Pawn>(PC->GetPawn());
						
						// Si encontramos la moto que sigue viva... ¡Este es el ganador legítimo!
						if (TempPawn && !TempPawn->bIsDead)
						{
							if (ADC_PlayerState* WinnerPS = PC->GetPlayerState<ADC_PlayerState>())
							{
								// --- 2. GUARDAR PUNTAJE EN EL GAME INSTANCE ---
								if (UDC_GameInstance* GI = Cast<UDC_GameInstance>(GetGameInstance()))
								{
									// Le decimos al GameInstance que anote una victoria física
									GI->AddWin(WinnerPS->GetPlayerName());
			
									// Actualizamos el PlayerState para que la UI se entere instantáneamente
									WinnerPS->RoundsWon = GI->GetWins(WinnerPS->GetPlayerName());
			
									UE_LOG(LogTemp, Warning, TEXT("¡El jugador %s gana la ronda! Victorias totales: %d"), *WinnerPS->GetPlayerName(), WinnerPS->RoundsWon);

									// Condición de Victoria Definitiva del Torneo
									if (WinnerPS->RoundsWon >= 3)
									{
										UE_LOG(LogTemp, Warning, TEXT("¡%s ES EL CAMPEÓN DEL TORNEO!"), *WinnerPS->GetPlayerName());
				
										// Limpiamos la memoria para que el próximo "Restart" sea una partida desde cero
										GI->ResetTournament(); 
									}
								}
							}

							TempPawn->Multicast_OnWin(); 
							break;
						}
					}
				}
				
				EndRound();
			}
		}
	}
}

void ADC_GameMode::EndRound()
{
	GetWorldTimerManager().ClearTimer(RoundTimerHandle);
	
	// Le damos 3 segundos de gracia al juego para que no se corte el cartel de victoria/derrota instantáneamente
	FTimerHandle UnusedHandle;
	GetWorldTimerManager().SetTimer(UnusedHandle, [this]()
	{
		if (GetWorld())
		{
			GetWorld()->ServerTravel(TEXT("?Restart"), true);
		}
	}, 3.0f, false);
}

AActor* ADC_GameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	// 1. Buscamos TODOS los Player Starts del mapa
	TArray<AActor*> FoundStarts;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerStart::StaticClass(), FoundStarts);

	// 2. Calculamos el Tag que deberíamos buscar para este jugador actual.
	// Como SpawnIndex arranca en 0, para el primer jugador buscará "P1", para el segundo "P2", etc.
	FName TargetTag = *FString::Printf(TEXT("P%d"), SpawnIndex + 1);

	// 3. Recorremos la lista buscando el actor que tenga la etiqueta correcta
	for (AActor* Actor : FoundStarts)
	{
		APlayerStart* Start = Cast<APlayerStart>(Actor);
		if (Start && Start->PlayerStartTag == TargetTag)
		{
			// Incrementamos el contador para que el próximo jugador busque el siguiente número
			SpawnIndex++;
			
			UE_LOG(LogTemp, Warning, TEXT("Asignando correctamente el Player Start con Tag legítimo: %s"), *TargetTag.ToString());
			return Start;
		}
	}

	// --- CAMINO DE SEGURIDAD (Fallback) ---
	// Si por algún error de tipeo en el editor el código no encuentra el tag "P1", "P2", etc.,
	// ejecutamos la lógica nativa para que el juego no se rompa ni crashee.
	UE_LOG(LogTemp, Error, TEXT("¡Cuidado! No se encontró un Player Start con el Tag: %s. Usando posición por defecto."), *TargetTag.ToString());
	return Super::ChoosePlayerStart_Implementation(Player);
}

