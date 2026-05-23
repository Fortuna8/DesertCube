// Fill out your copyright notice in the Description page of Project Settings.


#include "Menu.h"
#include "Components/Button.h"
#include "Components/VerticalBox.h"
#include "Components/EditableText.h"
#include "MultiplayerSessionsSubsystem.h"
#include "OnlineSessionSettings.h"
#include "OnlineSubsystem.h"
#include "WidgetSessionSerch.h"

void UMenu::ConfigurarMenu(int32 NumeroConexionesPublicasParam, FString TipoDePartidaParam, FString RutaLobbyParam)
{
	RutaAlLobby = FString::Printf(TEXT("%s?listen"), *RutaLobbyParam);
	NumeroConexionesPublicas = NumeroConexionesPublicasParam;
	TipoDePartida = TipoDePartidaParam;

	AddToViewport();
	SetVisibility(ESlateVisibility::Visible);
	SetIsFocusable(true);

	if (UWorld* World = GetWorld())
	{
		if (APlayerController* PC = World->GetFirstPlayerController())
		{
			FInputModeUIOnly InputModeData;
			InputModeData.SetWidgetToFocus(TakeWidget());
			InputModeData.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			PC->SetInputMode(InputModeData);
			PC->SetShowMouseCursor(true);
		}
	}

	if (UGameInstance* GI = GetGameInstance())
	{
		SubsistemaSesiones = GI->GetSubsystem<UMultiplayerSessionsSubsystem>();
	}

	if (SubsistemaSesiones)
	{
		SubsistemaSesiones->MultijugadorAlCrearSesionCompletada.AddDynamic(this, &ThisClass::AlCrearSesion);
		SubsistemaSesiones->MultijugadorAlBuscarSesionesCompletado.AddUObject(this, &ThisClass::AlBuscarSesiones);
		SubsistemaSesiones->MultijugadorAlUnirseASesionCompletado.AddUObject(this, &ThisClass::AlUnirseASesion);
		SubsistemaSesiones->MultijugadorAlDestruirSesionCompletada.AddDynamic(this, &ThisClass::AlDestruirSesion);
		SubsistemaSesiones->MultijugadorAlIniciarSesionCompletada.AddDynamic(this, &ThisClass::AlIniciarSesion);
	}
}

bool UMenu::Initialize()
{
	if (!Super::Initialize()) return false;

	if (BotonHost)
	{
		BotonHost->OnClicked.AddDynamic(this, &ThisClass::BotonHostClickeado);
	}
	if (BotonUnirse)
	{
		BotonUnirse->OnClicked.AddDynamic(this, &ThisClass::BotonUnirseClickeado);
	}
	return true;
}

void UMenu::NativeDestruct()
{
	DesmontarMenu();
	Super::NativeDestruct();
}

void UMenu::AlCrearSesion(bool bFueExitoso)
{
	if (bFueExitoso)
	{
		if (UWorld* World = GetWorld())
		{
			World->ServerTravel(RutaAlLobby);
		}
	}
	else
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Red, TEXT("Fallo al crear la sesion"));
		}
		if (BotonHost) BotonHost->SetIsEnabled(true);
	}
}

void UMenu::AlBuscarSesiones(const TArray<FOnlineSessionSearchResult>& ResultadosSesion, bool bFueExitoso)
{
	if (!SubsistemaSesiones)
	{
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, TEXT("AlBuscarSesiones: SubSistema nulo"));
		return;
	}

	if (GEngine)
	{
		const FString Msg = FString::Printf(TEXT("Buscando sesiones... exito: %s, Cantidad: %d"),
			bFueExitoso ? TEXT("Si") : TEXT("No"),
			ResultadosSesion.Num());
		GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Yellow, Msg);
	}

	if (CajaResultados) CajaResultados->ClearChildren();

	for (const auto& Resultado : ResultadosSesion)
	{
		const FString Id = Resultado.GetSessionIdStr();
		const FString Usuario = Resultado.Session.OwningUserName;
		if (GEngine)
			GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Cyan,
				FString::Printf(TEXT("ID: %s, User: %s"), *Id, *Usuario));

		if (!ClaseWidgetBusqueda) continue;

		OpcionSesion = CreateWidget<UWidgetSessionSerch>(GetWorld(), ClaseWidgetBusqueda);
		if (!OpcionSesion) continue;

		if (CajaResultados) CajaResultados->AddChildToVerticalBox(OpcionSesion);
		OpcionSesion->SetData(FText::FromString(Usuario));
		OpcionSesion->SetSession(Resultado);
		OpcionSesion->OnButtonSelectSession.AddUObject(this, &ThisClass::SeleccionarSesion);

		// OJO: clave alineada con el SubSistema (si cambiaste a "TipoPartida", usala aqui)
		FString FoundMatchType;
		Resultado.Session.SessionSettings.Get(FName("TipoPartida"), FoundMatchType);

		/* Ejemplo de autounirse por tipo de partida
		if (FoundMatchType == TipoDePartida)
		{
			if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Cyan, TEXT("Uniendo a sesion valida..."));
			SubsistemaSesiones->UnirseASesion(Resultado);
			return;
		}
		*/
	}

	if (!bFueExitoso || ResultadosSesion.Num() == 0)
	{
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, TEXT("No se encontraron sesiones validas"));
		if (BotonUnirse) BotonUnirse->SetIsEnabled(true);
	}
}

void UMenu::AlUnirseASesion(EOnJoinSessionCompleteResult::Type Resultado)
{
	FString ResultadoStr;
	switch (Resultado)
	{
	case EOnJoinSessionCompleteResult::Success: ResultadoStr = "Success"; break;
	case EOnJoinSessionCompleteResult::AlreadyInSession: ResultadoStr = "AlreadyInSession"; break;
	case EOnJoinSessionCompleteResult::SessionIsFull: ResultadoStr = "SessionIsFull"; break;
	case EOnJoinSessionCompleteResult::SessionDoesNotExist: ResultadoStr = "SessionDoesNotExist"; break;
	case EOnJoinSessionCompleteResult::CouldNotRetrieveAddress: ResultadoStr = "CouldNotRetrieveAddress"; break;
	case EOnJoinSessionCompleteResult::UnknownError:
	default: ResultadoStr = "UnknownError"; break;
	}

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Magenta,
			FString::Printf(TEXT("Resultado UnirseASesion: %s"), *ResultadoStr));
	}

	IOnlineSubsystem* Subsistema = IOnlineSubsystem::Get();
	if (!Subsistema)
	{
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, TEXT("OnlineSubsystem no encontrado"));
		return;
	}

	IOnlineSessionPtr InterfazSesion = Subsistema->GetSessionInterface();
	if (!InterfazSesion.IsValid())
	{
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, TEXT("InterfazSesion no valida"));
		return;
	}

	FString Direccion;
	if (InterfazSesion->GetResolvedConnectString(NAME_GameSession, Direccion))
	{
		if (GEngine)
			GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Green,
				FString::Printf(TEXT("Direccion: %s"), *Direccion));

		if (APlayerController* PC = GetGameInstance()->GetFirstLocalPlayerController())
		{
			PC->ClientTravel(Direccion, ETravelType::TRAVEL_Absolute);
		}
		else if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, TEXT("PlayerController nulo"));
		}
	}
	else
	{
		if (GEngine)
			GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, TEXT("No se pudo resolver la direccion de conexion"));
	}
}

void UMenu::AlDestruirSesion(bool /*bFueExitoso*/)
{
	// Implementar si necesitas feedback al destruir
}

void UMenu::AlIniciarSesion(bool /*bFueExitoso*/)
{
	// Implementar si necesitas feedback al iniciar
}

void UMenu::BotonHostClickeado()
{
	if (BotonHost) BotonHost->SetIsEnabled(false);

	// FIX: obtener texto escrito, no el nombre del objeto
	if (TextoNombreHost)
	{
		TipoDePartida = TextoNombreHost->GetText().ToString();
	}

	if (SubsistemaSesiones)
	{
		SubsistemaSesiones->CrearSesion(NumeroConexionesPublicas, TipoDePartida);
	}
}

void UMenu::BotonUnirseClickeado()
{
	if (BotonUnirse) BotonUnirse->SetIsEnabled(false);
	if (SubsistemaSesiones)
	{
		SubsistemaSesiones->BuscarSesiones(10000);
	}
}

void UMenu::DesmontarMenu()
{
	RemoveFromParent();
	if (UWorld* World = GetWorld())
	{
		if (APlayerController* PC = World->GetFirstPlayerController())
		{
			FInputModeGameOnly InputModeData;
			PC->SetInputMode(InputModeData);
			PC->SetShowMouseCursor(false);
		}
	}
}

void UMenu::SeleccionarSesion(const FOnlineSessionSearchResult& ResultadoSesion)
{
	if (SubsistemaSesiones)
	{
		SubsistemaSesiones->UnirseASesion(ResultadoSesion);
	}
}