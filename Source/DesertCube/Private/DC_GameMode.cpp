#include "DC_GameMode.h"
#include "DC_GameState.h"
#include "DC_PlayerState.h"
#include "DC_Pawn.h" // ¡IMPORTANTE PARA HABLARLE A LAS MOTOS!
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
				// Fuerza el fin de ronda para todos
				EndRound();
			}
			else
			{
				// Opcional: Si querés que el muerto sea espectador, acá lo seteas
				if (VictimController) VictimController->ChangeState(NAME_Spectating);
			}
		}
	}
}

void ADC_GameMode::EndRound()
{
	// 1. Frenamos el reloj
	GetWorldTimerManager().ClearTimer(RoundTimerHandle);
	
	// 2. Buscamos a TODOS los jugadores conectados y los congelamos
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (PC && PC->GetPawn())
		{
			if (ADC_Pawn* MyPawn = Cast<ADC_Pawn>(PC->GetPawn()))
			{
				MyPawn->Multicast_StopRound();
			}
		}
	}
	
	// 3. Un delay de 1.0 segundo antes de reiniciar el nivel para todos
	FTimerHandle UnusedHandle;
	GetWorldTimerManager().SetTimer(UnusedHandle, [this]()
	{
		if (GetWorld())
		{
			GetWorld()->ServerTravel(TEXT("?Restart"), true);
		}
	}, 1.0f, false);
}

AActor* ADC_GameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	TArray<AActor*> FoundStarts;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerStart::StaticClass(), FoundStarts);

	if (FoundStarts.Num() > 0)
	{
		int32 StartToUse = SpawnIndex % FoundStarts.Num();
		SpawnIndex++;
		
		UE_LOG(LogTemp, Warning, TEXT("Asignando el Player Start número: %d"), StartToUse);
		return FoundStarts[StartToUse];
	}

	return Super::ChoosePlayerStart_Implementation(Player);
}