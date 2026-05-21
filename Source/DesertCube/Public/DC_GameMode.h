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
	
	FTimerHandle RoundTimerHandle;

	void OnOneSecondPassed();
	void EndRound();
	
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;

	int32 SpawnIndex = 0;
};