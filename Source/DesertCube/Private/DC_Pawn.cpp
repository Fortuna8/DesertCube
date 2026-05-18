#include "DC_Pawn.h"
#include "DC_GameMode.h"
#include "DC_TrailSegment.h"
#include "Components/StaticMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"

ADC_Pawn::ADC_Pawn()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true; 
	SetReplicatingMovement(true);
	
	USceneComponent* RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	RootComponent = RootScene;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(RootScene);
	MeshComponent->SetGenerateOverlapEvents(true);
	
	MovementSpeed = 800.f;
	CurrentTargetYaw = 0.f;
	
	// Iniciamos vivos
	bIsDead = false; 
	
	// Iniciamos con el modo Snake activado y 20 metros de largo
	bIsTrailFinite = true;
	MaxTrailLength = 2000.f;
	
}

void ADC_Pawn::BeginPlay()
{
	Super::BeginPlay();
	
	// 1. SINCRONIZAMOS LA BRÚJULA: Leemos hacia dónde mira al nacer en el mapa
	CurrentTargetYaw = GetActorRotation().Yaw;
	
	if (HasAuthority()) 
	{
		MeshComponent->OnComponentBeginOverlap.AddDynamic(this, &ADC_Pawn::OnOverlapBegin);
	}
	
	SpawnNewSegment();
}

void ADC_Pawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	FVector ForwardMove = GetActorForwardVector() * MovementSpeed * DeltaTime;
	AddActorWorldOffset(ForwardMove, false);

	if (HasAuthority() && CurrentSegment)
	{
		CurrentSegment->UpdateSegment(LastTurnLocation, GetActorLocation());

		// Lógica del modo snake
		if (bIsTrailFinite && ActiveSegments.Num() > 0)
		{
			// 1. Calculamos cuánto mide la estela entera sumando todos los segmentos
			float TotalLength = 0.f;
			for (ADC_TrailSegment* Seg : ActiveSegments)
			{
				if (Seg) TotalLength += FVector::Distance(Seg->StartLoc, Seg->EndLoc);
			}

			// 2. Si nos pasamos del límite, empezamos a recortar desde la cola
			while (TotalLength > MaxTrailLength && ActiveSegments.Num() > 0)
			{
				ADC_TrailSegment* OldestSeg = ActiveSegments[0]; // Agarramos el más viejo
				
				// Limpieza por seguridad
				if (!OldestSeg)
				{
					ActiveSegments.RemoveAt(0);
					continue;
				}

				float Excess = TotalLength - MaxTrailLength;
				float OldestLen = FVector::Distance(OldestSeg->StartLoc, OldestSeg->EndLoc);

				// Si el exceso es mayor a lo que mide el segmento viejo (y no es el único que nos queda)
				if (Excess >= OldestLen && ActiveSegments.Num() > 1)
				{
					// Destruimos el segmento viejo por completo
					TotalLength -= OldestLen;
					OldestSeg->Destroy();
					ActiveSegments.RemoveAt(0);
				}
				else
				{
					// Si sobra menos, simplemente achicamos el segmento viejo moviendo su punto de inicio
					FVector Dir = (OldestSeg->EndLoc - OldestSeg->StartLoc).GetSafeNormal();
					FVector NewStart = OldestSeg->StartLoc + (Dir * Excess);
					OldestSeg->UpdateSegment(NewStart, OldestSeg->EndLoc);
					break; // Ya quedó del tamaño exacto, salimos del loop
				}
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
	
	// Filtramos inputs muy bajos para evitar "fantasmas"
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

	// 2. TOLERANCIA A DECIMALES: Usamos IsNearlyEqual
	bool bIsSameDirection = FMath::IsNearlyEqual(CurrentTargetYaw, NewYaw, 1.0f) || 
							(FMath::IsNearlyEqual(FMath::Abs(CurrentTargetYaw), 180.f, 1.0f) && FMath::IsNearlyEqual(FMath::Abs(NewYaw), 180.f, 1.0f));

	// Regla de Tron: No podés girar 180 grados de golpe
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
	SpawnNewSegment();
}

void ADC_Pawn::SpawnNewSegment()
{
	if (HasAuthority() && TrailClass)
	{
		LastTurnLocation = GetActorLocation();
		
		// 1. Preparamos el terreno
		FTransform SpawnTransform(FRotator::ZeroRotator, LastTurnLocation);
		
		// 2. SPAWN DIFERIDO: Empieza a crear el actor pero pausa su inicialización física
		ADC_TrailSegment* NewSegment = GetWorld()->SpawnActorDeferred<ADC_TrailSegment>(TrailClass, SpawnTransform, this);
		
		if (NewSegment)
		{
			// Guardamos el puntero ANTES de que evalúe colisiones
			CurrentSegment = NewSegment;
			
			// Inicializamos las coordenadas del segmento
			NewSegment->StartLoc = LastTurnLocation;
			NewSegment->EndLoc = LastTurnLocation;
			
			// Los agregamos a la lista
			ActiveSegments.Add(NewSegment);
			
			// Le decimos a Unreal que termine de armarlo y lance los eventos
			NewSegment->FinishSpawning(SpawnTransform);
		}
	}
}

void ADC_Pawn::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// Filtro 1: Si ya morimos en este frame, ignoramos el resto
	if (bIsDead) return;

	// Filtro 2: Ahora CurrentSegment sí es la pared nueva, así que nos va a perdonar la vida
	if (OtherActor && OtherActor != this && OtherActor != CurrentSegment)
	{
		if (OtherActor->IsA(ADC_TrailSegment::StaticClass()))
		{
			bIsDead = true; // Marcamos la muerte para evitar impresiones dobles
			
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