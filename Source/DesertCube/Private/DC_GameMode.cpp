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

	ADC_GameState* GS = GetGameState<ADC_GameState>();
	UDC_GameInstance* GI = Cast<UDC_GameInstance>(GetGameInstance());

	if (GS && GI)
	{
		GS->bGlobalIsTrailFinite = true; 
		GS->GlobalMaxTrailLength = 2000.f;

		// EL SERVIDOR INYECTA LA RONDA EN EL GAME STATE PARA QUE LLEGUE A TODOS
		GS->RondaActual = GI->RondaActual;
	}
}

void ADC_GameMode::OnPostLogin(AController* NewPlayer)
{
	Super::OnPostLogin(NewPlayer);

	// Englobamos todo en este chequeo maestro para que 'GS' exista para todo el código
	if (ADC_GameState* GS = GetGameState<ADC_GameState>())
	{
		if (ADC_PlayerState* PS = NewPlayer->GetPlayerState<ADC_PlayerState>())
		{
			// 1. OBTENEMOS EL ÍNDICE EXACTO (0, 1, 2 o 3) usando GS
			int32 Index = GS->PlayerArray.IndexOfByKey(PS);
				
			// 2. LE ASIGNAMOS EL NOMBRE SEGÚN SU COLOR
			FString NombreColor = TEXT("Player Blanco");
			switch (Index)
			{
			case 0: NombreColor = TEXT("Player Azul"); break;
			case 1: NombreColor = TEXT("Player Rojo"); break;
			case 2: NombreColor = TEXT("Player Verde"); break;
			case 3: NombreColor = TEXT("Player Amarillo"); break;
			}
				
			PS->SetPlayerName(NombreColor);

			// 3. RECUPERAMOS EL PUNTAJE DEL GAME INSTANCE CON SU NUEVO NOMBRE
			if (UDC_GameInstance* GI = Cast<UDC_GameInstance>(GetGameInstance()))
			{
				PS->RoundsWon = GI->GetWins(PS->GetPlayerName());
			}
		}
		
		// 4. LÓGICA DE JUGADORES CONECTADOS (Sigue usando el mismo GS)
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
	if (bRoundEnded) return;
	
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

			if (GS->PlayersAlive == 1)
			{
				// Queda 1 vivo. En lugar de darle la victoria ya, esperamos 0.05 segundos
				// por si fue un choque simultáneo frontal.
				if (!GetWorldTimerManager().IsTimerActive(PhotoFinishTimerHandle))
				{
					GetWorldTimerManager().SetTimer(PhotoFinishTimerHandle, this, &ADC_GameMode::EvaluateRoundWinner, 0.05f, false);
				}
			}
			else if (GS->PlayersAlive <= 0)
			{
				// Si llegó a 0 antes de que el timer termine, significa que ambos murieron
				// en el mismo fotograma. No hacemos nada, dejamos que el timer actúe.
				UE_LOG(LogTemp, Warning, TEXT("¡Muerte simultánea detectada en el mismo Tick!"));
			}
		}
	}
}

void ADC_GameMode::EndRound()
{
	if (bRoundEnded) return; 
	bRoundEnded = true;

	GetWorldTimerManager().ClearTimer(RoundTimerHandle);
	
	if (GetWorldTimerManager().IsTimerActive(MapTravelTimerHandle)) return;

	float WaitTime = 3.0f;

	// Si el torneo terminó, cambiamos las reglas de espera y avisamos a las interfaces
	if (bIsMatchOver)
	{
		WaitTime = 8.0f; // 8 segundos de gracia para leer el tablero de puntajes
		
		for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
		{
			if (APlayerController* PC = It->Get())
			{
				if (ADC_Pawn* MyPawn = Cast<ADC_Pawn>(PC->GetPawn()))
				{
					MyPawn->Multicast_MatchOver();
				}
			}
		}
	}

	GetWorldTimerManager().SetTimer(MapTravelTimerHandle, [this]()
	{
		if (GetWorld())
		{
			if (bIsMatchOver)
			{
				if (UDC_GameInstance* GI = Cast<UDC_GameInstance>(GetGameInstance()))
				{
					GI->ResetTournament();
				}
				GetWorld()->ServerTravel(TEXT("Lobby")); 
			}
			else
			{
				if (UDC_GameInstance* GI = Cast<UDC_GameInstance>(GetGameInstance()))
				{
					GI->RondaActual++;
				}
				GetWorld()->ServerTravel(TEXT("Basic")); 
			}
		}
	}, WaitTime, false);
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

void ADC_GameMode::EvaluateRoundWinner()
{
	ADC_GameState* GS = GetGameState<ADC_GameState>();
	if (!GS) return;

	// Si después del Photo Finish sigue quedando 1 vivo, hay un ganador real
	if (GS->PlayersAlive == 1)
	{
		for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
		{
			APlayerController* PC = It->Get();
			if (PC && PC->GetPawn())
			{
				ADC_Pawn* TempPawn = Cast<ADC_Pawn>(PC->GetPawn());
						
				if (TempPawn && !TempPawn->bIsDead)
				{
					if (ADC_PlayerState* WinnerPS = PC->GetPlayerState<ADC_PlayerState>())
					{
						if (UDC_GameInstance* GI = Cast<UDC_GameInstance>(GetGameInstance()))
						{
							GI->AddWin(WinnerPS->GetPlayerName());
							WinnerPS->RoundsWon = GI->GetWins(WinnerPS->GetPlayerName());
			
							UE_LOG(LogTemp, Warning, TEXT("¡El jugador %s gana la ronda! Victorias: %d"), *WinnerPS->GetPlayerName(), WinnerPS->RoundsWon);

							if (WinnerPS->RoundsWon >= TargetWins)
							{
								bIsMatchOver = true; 
								UE_LOG(LogTemp, Warning, TEXT("¡%s ES EL CAMPEÓN DEL TORNEO!"), *WinnerPS->GetPlayerName());
								GI->ResetTournament();
							}
						}
					}
					TempPawn->Multicast_OnWin(); 
					break;
				}
			}
		}
	}
	// Si los jugadores vivos llegaron a 0, nadie gana.
	else if (GS->PlayersAlive <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("¡EMPATE TÉCNICO! Nadie suma puntos esta ronda."));
		
		// Opcional: Podrías hacer un Multicast_Draw() a todos los pawns si querés 
		// mostrar un cartel de "EMPATE" en la UI antes de reiniciar.
	}

	// Terminamos la ronda pase lo que pase
	EndRound();
}

void ADC_GameMode::Logout(AController* Exiting)
{
	if (ADC_GameState* GS = GetGameState<ADC_GameState>())
	{
		if (ADC_PlayerState* PS = Exiting->GetPlayerState<ADC_PlayerState>())
		{
			// Si el jugador estaba corriendo en la arena, lo restamos del contador
			if (PS->bIsAlive)
			{
				PS->bIsAlive = false;
				GS->PlayersAlive = FMath::Max(0, GS->PlayersAlive - 1);
				UE_LOG(LogTemp, Warning, TEXT("El jugador %s abandonó la partida. Jugadores restantes: %d"), *PS->GetPlayerName(), GS->PlayersAlive);

				// Reutilizamos tu lógica del Photo Finish por si su abandono termina la ronda
				if (GS->PlayersAlive <= 1)
				{
					if (!GetWorldTimerManager().IsTimerActive(PhotoFinishTimerHandle))
					{
						GetWorldTimerManager().SetTimer(PhotoFinishTimerHandle, this, &ADC_GameMode::EvaluateRoundWinner, 0.05f, false);
					}
				}
			}
		}
	}

	// Súper importante llamar al nativo para que Unreal libere la memoria de ese jugador
	Super::Logout(Exiting); 
}