#include "DC_Pawn.h"
#include "DC_GameMode.h"
#include "DC_GameState.h"
#include "DC_TrailLine.h"
#include "Components/StaticMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Components/BoxComponent.h"
#include "Net/UnrealNetwork.h"
#include "EngineUtils.h" // NECESARIO PARA BUSCAR ACTORES

ADC_Pawn::ADC_Pawn()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true; 
	SetReplicatingMovement(true);
	
	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
	RootComponent = CollisionBox;
	
	// La caja ahora solo se usa para rebotar contra los muros externos del mapa
	CollisionBox->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(RootComponent);
	MeshComponent->SetGenerateOverlapEvents(false);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	
	// Arranca congelado por el Warmup 3,2,1
	MovementSpeed = 0.f;
	CurrentTargetYaw = 0.f;
	bIsDead = false; 
	bIsTrailFinite = true;
	MaxTrailLength = 2000.f;
	bDieOnWallCollision = true;
}


// GENERAL
void ADC_Pawn::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ADC_Pawn, MyTrailLine);
}

void ADC_Pawn::BeginPlay()
{
	Super::BeginPlay();
	CurrentTargetYaw = GetActorRotation().Yaw;
	
	if (HasAuthority()) 
	{
		InitializeTrail();
	}
}

void ADC_Pawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bIsDead || MovementSpeed <= 0.f) return;

	FVector OldLocation = GetActorLocation();
	FVector ForwardMove = GetActorForwardVector() * MovementSpeed * DeltaTime;
	
	FHitResult HitResult;
	// 1. Nos movemos físicamente (Chequeamos choque con paredes del mapa)
	AddActorWorldOffset(ForwardMove, true, &HitResult);
	FVector NewLocation = GetActorLocation();

	if (HitResult.bBlockingHit && bDieOnWallCollision && HasAuthority())
	{
		Die();
		return;
	}

	// 2. ESCÁNER MATEMÁTICO: Chequeamos contra todas las estelas de los jugadores (Solo el Servidor juzga)
	if (HasAuthority())
	{
		// Escaneamos todas las líneas de luz en el nivel
		for (TActorIterator<ADC_TrailLine> It(GetWorld()); It; ++It)
		{
			ADC_TrailLine* Line = *It;
			float BikeRadius = 40.0f; // El grosor de tu moto
			
			if (Line->CheckMathematicalCollision(OldLocation, NewLocation, BikeRadius, this))
			{
				Die();
				break; 
			}
		}
	}
}

void ADC_Pawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	
	if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			if (DefaultMappingContext) Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (MoveAction) EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ADC_Pawn::Move);
	}
}


// MOVIMIENTO

void ADC_Pawn::Move(const FInputActionValue& Value)
{
	FVector2D MovementVector = Value.Get<FVector2D>();
	if (MovementVector.SizeSquared() < 0.1f) return;

	float NewYaw = CurrentTargetYaw;

	if (FMath::Abs(MovementVector.X) > FMath::Abs(MovementVector.Y))
	{
		NewYaw = (MovementVector.X > 0) ? 90.f : -90.f; 
	}
	else 
	{
		NewYaw = (MovementVector.Y > 0) ? 0.f : 180.f;  
	}

	bool bIsSameDirection = FMath::IsNearlyEqual(CurrentTargetYaw, NewYaw, 1.0f) || 
							(FMath::IsNearlyEqual(FMath::Abs(CurrentTargetYaw), 180.f, 1.0f) && FMath::IsNearlyEqual(FMath::Abs(NewYaw), 180.f, 1.0f));

	float YawDiff = FMath::Abs(CurrentTargetYaw - NewYaw);
	bool bIsOpposite = FMath::IsNearlyEqual(YawDiff, 180.f, 1.0f) || FMath::IsNearlyEqual(YawDiff, 540.f, 1.0f);

	if (!bIsSameDirection && !bIsOpposite)
	{
		CurrentTargetYaw = NewYaw;
		SetActorRotation(FRotator(0.f, CurrentTargetYaw, 0.f));
		Server_Turn(CurrentTargetYaw);
	}
}

void ADC_Pawn::Server_Turn_Implementation(float NewYaw)
{
	SetActorRotation(FRotator(0.f, NewYaw, 0.f));
	if (MyTrailLine) MyTrailLine->Multicast_AddTurnPoint(GetActorLocation());
}

void ADC_Pawn::InitializeTrail()
{
	if (HasAuthority() && TrailLineClass)
	{
		FTransform SpawnTransform(FRotator::ZeroRotator, GetActorLocation());
		MyTrailLine = GetWorld()->SpawnActor<ADC_TrailLine>(TrailLineClass, SpawnTransform);

		if (MyTrailLine)
		{
			MyTrailLine->TargetPawn = this; 
			MyTrailLine->InitialPoint = GetActorLocation(); 
			MyTrailLine->TurnCorners.Add(GetActorLocation());
			MyTrailLine->bIsInitialized = true;
		}
	}
}


// ESTADOS DE RONDA
void ADC_Pawn::Multicast_StartRound_Implementation()
{
	MovementSpeed = 800.f;
}

void ADC_Pawn::Multicast_StopRound_Implementation()
{
	MovementSpeed = 0.f;
	
	if (IsLocallyControlled())
	{
		if (APlayerController* PC = Cast<APlayerController>(GetController()))
		{
			DisableInput(PC);
		}
	}
}

void ADC_Pawn::Die()
{
	// 1. Le avisamos al Game Mode (Solo el servidor puede hacer esto)
	if (HasAuthority())
	{
		// Buscamos el Game Mode y lo casteamos a nuestra clase
		if (ADC_GameMode* GM = Cast<ADC_GameMode>(GetWorld()->GetAuthGameMode()))
		{
			// Le pasamos el controlador de esta moto para que lo procese
			GM->PlayerDied(GetController());
		}
	}

	// 2. Ejecutamos el Multicast para los efectos visuales de la muerte
	Multicast_Die();
}

void ADC_Pawn::Multicast_Die_Implementation()
{
	bIsDead = true;
	MovementSpeed = 0.f;
	MeshComponent->SetHiddenInGame(true);
    
	// Si yo soy el dueño de este pawn, desactivo los inputs
	if (IsLocallyControlled())
	{
		DisableInput(Cast<APlayerController>(GetController()));
	}
}