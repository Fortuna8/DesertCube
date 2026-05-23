#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"

#include "MultiplayerSessionsSubsystem.generated.h"

//
// Declaramos nuestros propios delegados para que Menu pueda enlazar callbacks
//
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMultijugadorAlCrearSesionCompletada, bool, bFueExitoso);
DECLARE_MULTICAST_DELEGATE_TwoParams(FMultijugadorAlBuscarSesionesCompletado, const TArray<FOnlineSessionSearchResult>& ResultadosBusqueda, bool bFueExitoso);
DECLARE_MULTICAST_DELEGATE_OneParam(FMultijugadorAlUnirseASesionCompletado, EOnJoinSessionCompleteResult::Type Resultado);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMultijugadorAlDestruirSesionCompletada, bool, bFueExitoso);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMultijugadorAlIniciarSesionCompletada, bool, bFueExitoso);

/**
 *
 */
UCLASS()
class MULTIPLAYERSESSIONOW_API UMultiplayerSessionsSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	UMultiplayerSessionsSubsystem();

	//
	// Manejo de sesiones. Menu llamara estas funciones
	//
	void CrearSesion(int32 NumConexionesPublicas, FString TipoPartida);
	void BuscarSesiones(int32 MaxResultadosBusqueda);
	void UnirseASesion(const FOnlineSessionSearchResult& ResultadoSesion);
	void DestruirSesion();
	void IniciarSesion();

	bool EsValidaInterfazSesion();

	//
	// Nuestros delegados para que Menu se pueda suscribir
	//
	FMultijugadorAlCrearSesionCompletada MultijugadorAlCrearSesionCompletada;
	FMultijugadorAlBuscarSesionesCompletado MultijugadorAlBuscarSesionesCompletado;
	FMultijugadorAlUnirseASesionCompletado MultijugadorAlUnirseASesionCompletado;
	FMultijugadorAlDestruirSesionCompletada MultijugadorAlDestruirSesionCompletada;
	FMultijugadorAlIniciarSesionCompletada MultijugadorAlIniciarSesionCompletada;

protected:

	//
	// Callbacks internos que se agregan a la lista de delegados de Online Session Interface.
	// No necesitan ser llamados fuera de esta clase.
	//
	void AlCrearSesionCompletada(FName NombreSesion, bool bFueExitoso);
	void AlBuscarSesionesCompletado(bool bFueExitoso);
	void AlUnirseASesionCompletado(FName NombreSesion, EOnJoinSessionCompleteResult::Type Resultado);
	void AlDestruirSesionCompletada(FName NombreSesion, bool bFueExitoso);
	void AlIniciarSesionCompletada(FName NombreSesion, bool bFueExitoso);

private:
	IOnlineSessionPtr InterfazSesion;
	TSharedPtr<FOnlineSessionSettings> UltimasConfiguracionesSesion;
	TSharedPtr<FOnlineSessionSearch> UltimaBusquedaSesion;

	//
	// Delegados para agregar a la interfaz de sesiones online.
	// Enlazamos nuestros callbacks internos a estos.
	//
	FOnCreateSessionCompleteDelegate CrearSesionCompletadaDelegado;
	FDelegateHandle CrearSesionCompletadaHandle;
	FOnFindSessionsCompleteDelegate BuscarSesionesCompletadoDelegado;
	FDelegateHandle BuscarSesionesCompletadoHandle;
	FOnJoinSessionCompleteDelegate UnirseASesionCompletadoDelegado;
	FDelegateHandle UnirseASesionCompletadoHandle;
	FOnDestroySessionCompleteDelegate DestruirSesionCompletadaDelegado;
	FDelegateHandle DestruirSesionCompletadaHandle;
	FOnStartSessionCompleteDelegate IniciarSesionCompletadaDelegado;
	FDelegateHandle IniciarSesionCompletadaHandle;

	bool bCrearSesionAlDestruir{ false };
	int32 UltimoNumConexionesPublicas;
	FString UltimoTipoPartida;
};
