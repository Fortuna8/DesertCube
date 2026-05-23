// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "OnlineSessionSettings.h"
#include "Blueprint/UserWidget.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "WidgetSessionSerch.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FOnButtonSelectSession, const FOnlineSessionSearchResult& SessionButton);
/**
 * 
 */
UCLASS()
class MULTIPLAYERSESSIONOW_API UWidgetSessionSerch : public UUserWidget
{
	GENERATED_BODY()


	virtual void NativeConstruct() override;

public:
	
	UPROPERTY(meta = (BindWidget))
		class UButton* ConectSessionButton;

	UPROPERTY(meta = (BindWidget))
		class UTextBlock* UserName;

	UFUNCTION()
		void SetData(FText UserN);

	UFUNCTION()
	void OnButtonClicked();

	
	void SetSession(FOnlineSessionSearchResult Session);


	FOnlineSessionSearchResult SerchSession;

	
	FOnButtonSelectSession OnButtonSelectSession;
};
