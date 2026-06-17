#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "DC_GameMode.generated.h"

UCLASS()
class DESERTCUBE_API ADC_GameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ADC_GameMode();

	UFUNCTION(BlueprintCallable, Category = "Game Rules")
	void PlayerDied(AController* VictimController);

protected:
	virtual void BeginPlay() override;
	virtual void OnPostLogin(AController* NewPlayer) override;
	
	// --- VARIABLES DE LA RONDA ---
	FTimerHandle RoundTimerHandle;
	void OnOneSecondPassed();
	void EndRound();
	
	// --- LÓGICA DE EMPATES (PHOTO FINISH) ---
	FTimerHandle PhotoFinishTimerHandle;
	void EvaluateRoundWinner();
	
	// --- NUEVO: SISTEMA DE CALENTAMIENTO ---
	FTimerHandle WarmupTimerHandle;
	int32 WarmupTime = 3;
	void OnWarmupTick();
	void StartMatch();

	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;

	int32 SpawnIndex = 0;
	
	bool bIsMatchOver = false;
	int32 TargetWins = 3;
};