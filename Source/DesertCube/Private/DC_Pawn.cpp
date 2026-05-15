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

// Called when the game starts or when spawned
void ADC_Pawn::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ADC_Pawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void ADC_Pawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

