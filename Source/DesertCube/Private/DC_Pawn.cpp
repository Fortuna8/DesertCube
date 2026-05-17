// Fill out your copyright notice in the Description page of Project Settings.


#include "DC_Pawn.h"
#include "Components/StaticMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"



ADC_Pawn::ADC_Pawn()
{
	PrimaryActorTick.bCanEverTick = true;
	
	bReplicates = true; 
	SetReplicatingMovement(true);
	
	// Mesh de cubo
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	RootComponent = MeshComponent;
	
	// Velocidad inicial
	MovementSpeed = 800.f;
}

void ADC_Pawn::BeginPlay()
{
	Super::BeginPlay();
	
}

void ADC_Pawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Calculamos el movimiento hacia adelante basado en la rotación actual
	FVector ForwardMove = GetActorForwardVector() * MovementSpeed * DeltaTime;
	
	// Aplicamos el movimiento. 
	// Nota: El 'true' activa el Sweep (colisiones). Si el cubo no se mueve después de compilar, 
	// cambialo a 'false' temporalmente para descartar que esté atascado en el piso.
	AddActorWorldOffset(ForwardMove, true);
}

void ADC_Pawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	
	// 1. Obtenemos el controlador del jugador
	if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		// 2. Obtenemos el subsistema local y agregamos el Mapping Context
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			if (DefaultMappingContext)
			{
				Subsystem->AddMappingContext(DefaultMappingContext, 0);
			}
		}
	}

	// 3. Vinculamos las acciones (Esto ya lo teníamos)
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (MoveAction)
		{
			EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ADC_Pawn::Move);
		}
	}
}

void ADC_Pawn::Move(const FInputActionValue& Value)
{
	FVector2D MovementVector = Value.Get<FVector2D>();

	// Verificamos cuál eje tiene mayor magnitud para evitar diagonales
	if (FMath::Abs(MovementVector.X) > FMath::Abs(MovementVector.Y))
	{
		// Eje X: Derecha / Izquierda
		float Yaw = (MovementVector.X > 0) ? 90.f : -90.f;
		SetActorRotation(FRotator(0.f, Yaw, 0.f));
	}
	else if (FMath::Abs(MovementVector.Y) > FMath::Abs(MovementVector.X))
	{
		// Eje Y: Arriba / Abajo
		float Yaw = (MovementVector.Y > 0) ? 0.f : 180.f;
		SetActorRotation(FRotator(0.f, Yaw, 0.f));
	}
}