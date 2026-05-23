// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Menu.generated.h"

UCLASS()
class MULTIPLAYERSESSIONOW_API UMenu : public UUserWidget
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable)
	void ConfigurarMenu(
		int32 NumeroConexionesPublicas = 4,
		FString TipoDePartida = FString(TEXT("KenoTeach")),
		FString RutaLobby = FString(TEXT("/Game/ThirdPersonCPP/Maps/Lobby"))
	);

	UPROPERTY()
	class UWidgetSessionSerch* OpcionSesion;

	// SUGERIDO: usar la clase concreta del widget de item de busqueda
	UPROPERTY(EditAnywhere, Category = "Busqueda de sesiones")
	TSubclassOf<class UWidgetSessionSerch> ClaseWidgetBusqueda;

protected:
	virtual bool Initialize() override;
	virtual void NativeDestruct() override;

	// Callbacks para los delegados del SubSistema de Sesiones
	UFUNCTION()
	void AlCrearSesion(bool bFueExitoso);
	void AlBuscarSesiones(const TArray<FOnlineSessionSearchResult>& ResultadosSesion, bool bFueExitoso);
	void AlUnirseASesion(EOnJoinSessionCompleteResult::Type Resultado);
	UFUNCTION()
	void AlDestruirSesion(bool bFueExitoso);
	UFUNCTION()
	void AlIniciarSesion(bool bFueExitoso);

private:
	UPROPERTY(meta = (BindWidget))
	class UButton* BotonHost;

	UPROPERTY(meta = (BindWidget))
	UButton* BotonUnirse;

	UPROPERTY(meta = (BindWidget))
	class UVerticalBox* CajaResultados;

	UPROPERTY(meta = (BindWidget))
	class UEditableText* TextoNombreHost;

	UFUNCTION()
	void BotonHostClickeado();

	UFUNCTION()
	void BotonUnirseClickeado();

	void DesmontarMenu();

	void SeleccionarSesion(const FOnlineSessionSearchResult& ResultadoSesion);

	// SubSistema para manejar sesiones online
	class UMultiplayerSessionsSubsystem* SubsistemaSesiones;

	int32 NumeroConexionesPublicas{ 4 };
	FString TipoDePartida{ TEXT("KenoTeach") };
	FString RutaAlLobby{ TEXT("") };
};
