// Fill out your copyright notice in the Description page of Project Settings.


#include "MultiplayerSessionsSubsystem.h"
#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"
#include "Online/OnlineSessionNames.h"

UMultiplayerSessionsSubsystem::UMultiplayerSessionsSubsystem() :
	CrearSesionCompletadaDelegado(FOnCreateSessionCompleteDelegate::CreateUObject(this, &ThisClass::AlCrearSesionCompletada)),
	BuscarSesionesCompletadoDelegado(FOnFindSessionsCompleteDelegate::CreateUObject(this, &ThisClass::AlBuscarSesionesCompletado)),
	UnirseASesionCompletadoDelegado(FOnJoinSessionCompleteDelegate::CreateUObject(this, &ThisClass::AlUnirseASesionCompletado)),
	DestruirSesionCompletadaDelegado(FOnDestroySessionCompleteDelegate::CreateUObject(this, &ThisClass::AlDestruirSesionCompletada)),
	IniciarSesionCompletadaDelegado(FOnStartSessionCompleteDelegate::CreateUObject(this, &ThisClass::AlIniciarSesionCompletada))
{
}

void UMultiplayerSessionsSubsystem::CrearSesion(int32 NumConexionesPublicas, FString TipoPartida)
{
	if (!EsValidaInterfazSesion())
	{
		return;
	}

	auto SesionExistente = InterfazSesion->GetNamedSession(NAME_GameSession);
	if (SesionExistente != nullptr)
	{
		bCrearSesionAlDestruir = true;
		UltimoNumConexionesPublicas = NumConexionesPublicas;
		UltimoTipoPartida = TipoPartida;

		DestruirSesion();
		// Nota: retornamos aqui? el flujo original no retornaba; se mantiene igual.
	}

	// Guardamos el handle para luego removerlo
	CrearSesionCompletadaHandle = InterfazSesion->AddOnCreateSessionCompleteDelegate_Handle(CrearSesionCompletadaDelegado);

	UltimasConfiguracionesSesion = MakeShareable(new FOnlineSessionSettings());
	UltimasConfiguracionesSesion->bIsLANMatch = IOnlineSubsystem::Get()->GetSubsystemName() == "NULL" ? true : false;
	UltimasConfiguracionesSesion->NumPublicConnections = NumConexionesPublicas;
	UltimasConfiguracionesSesion->bAllowJoinInProgress = true;
	UltimasConfiguracionesSesion->bAllowJoinViaPresence = true;
	UltimasConfiguracionesSesion->bShouldAdvertise = true;
	UltimasConfiguracionesSesion->bUsesPresence = true;
	UltimasConfiguracionesSesion->bUseLobbiesIfAvailable = true;
	UltimasConfiguracionesSesion->Set(FName("TipoPartida"), TipoPartida, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	UltimasConfiguracionesSesion->BuildUniqueId = 1;

	const ULocalPlayer* JugadorLocal = GetWorld()->GetFirstLocalPlayerFromController();
	if (!InterfazSesion->CreateSession(*JugadorLocal->GetPreferredUniqueNetId(), NAME_GameSession, *UltimasConfiguracionesSesion))
	{
		InterfazSesion->ClearOnCreateSessionCompleteDelegate_Handle(CrearSesionCompletadaHandle);

		// Emitimos nuestro delegado
		MultijugadorAlCrearSesionCompletada.Broadcast(false);
	}
}

void UMultiplayerSessionsSubsystem::BuscarSesiones(int32 MaxResultadosBusqueda)
{
	if (!EsValidaInterfazSesion())
	{
		return;
	}

	BuscarSesionesCompletadoHandle = InterfazSesion->AddOnFindSessionsCompleteDelegate_Handle(BuscarSesionesCompletadoDelegado);

	UltimaBusquedaSesion = MakeShareable(new FOnlineSessionSearch());
	UltimaBusquedaSesion->MaxSearchResults = MaxResultadosBusqueda;
	UltimaBusquedaSesion->bIsLanQuery = IOnlineSubsystem::Get()->GetSubsystemName() == "NULL" ? true : false;
	UltimaBusquedaSesion->QuerySettings.Set(SEARCH_LOBBIES, true, EOnlineComparisonOp::Equals);

	const ULocalPlayer* JugadorLocal = GetWorld()->GetFirstLocalPlayerFromController();
	if (!InterfazSesion->FindSessions(*JugadorLocal->GetPreferredUniqueNetId(), UltimaBusquedaSesion.ToSharedRef()))
	{
		InterfazSesion->ClearOnFindSessionsCompleteDelegate_Handle(BuscarSesionesCompletadoHandle);

		MultijugadorAlBuscarSesionesCompletado.Broadcast(TArray<FOnlineSessionSearchResult>(), false);
	}
}

void UMultiplayerSessionsSubsystem::UnirseASesion(const FOnlineSessionSearchResult& ResultadoSesion)
{
	if (!InterfazSesion.IsValid())
	{
		MultijugadorAlUnirseASesionCompletado.Broadcast(EOnJoinSessionCompleteResult::UnknownError);
		return;
	}

	UnirseASesionCompletadoHandle = InterfazSesion->AddOnJoinSessionCompleteDelegate_Handle(UnirseASesionCompletadoDelegado);

	const ULocalPlayer* JugadorLocal = GetWorld()->GetFirstLocalPlayerFromController();
	if (!InterfazSesion->JoinSession(*JugadorLocal->GetPreferredUniqueNetId(), NAME_GameSession, ResultadoSesion))
	{
		InterfazSesion->ClearOnJoinSessionCompleteDelegate_Handle(UnirseASesionCompletadoHandle);

		MultijugadorAlUnirseASesionCompletado.Broadcast(EOnJoinSessionCompleteResult::UnknownError);
	}
}

void UMultiplayerSessionsSubsystem::DestruirSesion()
{
	if (!InterfazSesion.IsValid())
	{
		MultijugadorAlDestruirSesionCompletada.Broadcast(false);
		return;
	}

	DestruirSesionCompletadaHandle = InterfazSesion->AddOnDestroySessionCompleteDelegate_Handle(DestruirSesionCompletadaDelegado);

	if (!InterfazSesion->DestroySession(NAME_GameSession))
	{
		InterfazSesion->ClearOnDestroySessionCompleteDelegate_Handle(DestruirSesionCompletadaHandle);
		MultijugadorAlDestruirSesionCompletada.Broadcast(false);
	}
}

void UMultiplayerSessionsSubsystem::IniciarSesion()
{
	// (vacío, igual que el original)
}

bool UMultiplayerSessionsSubsystem::EsValidaInterfazSesion()
{
	if (!InterfazSesion)
	{
		IOnlineSubsystem* Subsistema = IOnlineSubsystem::Get();
		if (Subsistema)
		{
			InterfazSesion = Subsistema->GetSessionInterface();
		}
	}
	return InterfazSesion.IsValid();
}

void UMultiplayerSessionsSubsystem::AlCrearSesionCompletada(FName NombreSesion, bool bFueExitoso)
{
	if (InterfazSesion)
	{
		InterfazSesion->ClearOnCreateSessionCompleteDelegate_Handle(CrearSesionCompletadaHandle);
	}

	MultijugadorAlCrearSesionCompletada.Broadcast(bFueExitoso);
}

void UMultiplayerSessionsSubsystem::AlBuscarSesionesCompletado(bool bFueExitoso)
{
	if (InterfazSesion)
	{
		InterfazSesion->ClearOnFindSessionsCompleteDelegate_Handle(BuscarSesionesCompletadoHandle);
	}

	if (UltimaBusquedaSesion->SearchResults.Num() <= 0)
	{
		MultijugadorAlBuscarSesionesCompletado.Broadcast(TArray<FOnlineSessionSearchResult>(), false);
		return;
	}

	MultijugadorAlBuscarSesionesCompletado.Broadcast(UltimaBusquedaSesion->SearchResults, bFueExitoso);
}

void UMultiplayerSessionsSubsystem::AlUnirseASesionCompletado(FName NombreSesion, EOnJoinSessionCompleteResult::Type Resultado)
{
	if (InterfazSesion)
	{
		InterfazSesion->ClearOnJoinSessionCompleteDelegate_Handle(UnirseASesionCompletadoHandle);
	}

	MultijugadorAlUnirseASesionCompletado.Broadcast(Resultado);
}

void UMultiplayerSessionsSubsystem::AlDestruirSesionCompletada(FName NombreSesion, bool bFueExitoso)
{
	if (InterfazSesion)
	{
		InterfazSesion->ClearOnDestroySessionCompleteDelegate_Handle(DestruirSesionCompletadaHandle);
	}
	if (bFueExitoso && bCrearSesionAlDestruir)
	{
		bCrearSesionAlDestruir = false;
		CrearSesion(UltimoNumConexionesPublicas, UltimoTipoPartida);
	}
	MultijugadorAlDestruirSesionCompletada.Broadcast(bFueExitoso);
}

void UMultiplayerSessionsSubsystem::AlIniciarSesionCompletada(FName NombreSesion, bool bFueExitoso)
{
	// (vacío, igual que el original)
}