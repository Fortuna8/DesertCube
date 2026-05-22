#include "DC_Pawn.h"
#include "DC_GameMode.h"
#include "DC_GameState.h"
#include "DC_TrailLine.h"
#include "Components/StaticMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Components/BoxComponent.h"
#include "Net/UnrealNetwork.h"

ADC_Pawn::ADC_Pawn()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true; 
	SetReplicatingMovement(true);
	
	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
	RootComponent = CollisionBox;
	CollisionBox->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	CollisionBox->SetGenerateOverlapEvents(true);
	
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(RootComponent);
	MeshComponent->SetGenerateOverlapEvents(false);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	
	// ---> MAGIA: ¡Arrancamos a velocidad cero! <---
	MovementSpeed = 0.f;
	CurrentTargetYaw = 0.f;
	bIsDead = false; 
	bIsTrailFinite = true;
	MaxTrailLength = 2000.f;
	bDieOnWallCollision = true;
}

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
		CollisionBox->OnComponentBeginOverlap.AddDynamic(this, &ADC_Pawn::OnOverlapBegin);
		InitializeTrail();
	}
}

void ADC_Pawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// SE FUE TODA LA LÓGICA DE LA LÍNEA. Solo nos movemos y chocamos la pared
	FHitResult HitResult;
	FVector ForwardMove = GetActorForwardVector() * MovementSpeed * DeltaTime;
	AddActorWorldOffset(ForwardMove, true, &HitResult);

	if (HitResult.bBlockingHit && bDieOnWallCollision && !bIsDead)
	{
		if (HasAuthority())
		{
			bIsDead = true;
			if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, TEXT("MUERTE: Te estrellaste contra el muro"));
			if (ADC_GameMode* GM = Cast<ADC_GameMode>(GetWorld()->GetAuthGameMode()))
			{
				GM->PlayerDied(GetController());
			}
			MovementSpeed = 0.f;
			MeshComponent->SetHiddenInGame(true);
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
			if (DefaultMappingContext)
			{
				Subsystem->AddMappingContext(DefaultMappingContext, 0);
			}
		}
	}

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
	
	if (MyTrailLine)
	{
		// El Servidor lanza el grito, y TODAS las PCs ejecutan el AddTurnPoint localmente
		MyTrailLine->Multicast_AddTurnPoint(GetActorLocation());
	}
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
			
			// EL CANDADO: Seteamos la variable garantizada para vencer al lag
			MyTrailLine->InitialPoint = GetActorLocation(); 
			
			// El servidor se auto-inicializa localmente
			MyTrailLine->TurnCorners.Add(GetActorLocation());
			MyTrailLine->bIsInitialized = true;
		}
	}
}

void ADC_Pawn::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (bIsDead) return;

	if (OtherActor && OtherActor != this)
	{
		if (OtherActor->IsA(ADC_TrailLine::StaticClass()))
		{
			ADC_TrailLine* HitLine = Cast<ADC_TrailLine>(OtherActor);
			
			if (HitLine == MyTrailLine)
			{
				if (HitLine->IsSafeSegment(OtherComp)) return; 
			}

			bIsDead = true; 
			if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, FString::Printf(TEXT("MUERTE: Chocaste con %s"), *OtherActor->GetName()));
			
			if (ADC_GameMode* GM = Cast<ADC_GameMode>(GetWorld()->GetAuthGameMode()))
			{
				GM->PlayerDied(GetController());
			}
			
			MovementSpeed = 0.f;
			MeshComponent->SetHiddenInGame(true);
		}
	}	
}

void ADC_Pawn::Multicast_StartRound_Implementation()
{
	// Cuando el GameMode grita "GO", todos los jugadores activan su velocidad a la vez
	MovementSpeed = 100.f;
}