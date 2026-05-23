// Fill out your copyright notice in the Description page of Project Settings.


#include "WidgetSessionSerch.h"
#include "Components/TextBlock.h"
#include "OnlineSessionSettings.h"
#include "OnlineSubsystem.h"
#include "Components/Button.h"

void UWidgetSessionSerch::NativeConstruct()
{
	Super::NativeConstruct();

	ConectSessionButton->OnClicked.AddUniqueDynamic(this, &ThisClass::OnButtonClicked);
}

void UWidgetSessionSerch::SetData(FText UserN)
{
	UserName->SetText(UserN);
}

void UWidgetSessionSerch::OnButtonClicked()
{
	
	OnButtonSelectSession.Broadcast(SerchSession);
}

void UWidgetSessionSerch::SetSession(FOnlineSessionSearchResult Session)
{
	SerchSession = Session;
}

