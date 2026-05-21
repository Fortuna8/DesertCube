#include "DC_Pawn.h"
#include "DC_GameMode.h"
#include "DC_GameState.h"
#include "DC_TrailSegment.h"
#include "Components/StaticMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Components/BoxComponent.h"

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
	
	MovementSpeed = 800.f;
	CurrentTargetYaw = 0.f;
	
	bIsDead = false; 
	bIsTrailFinite = true;
	MaxTrailLength = 2000.f;
	bDieOnWallCollision = true;
}

void ADC_Pawn::BeginPlay()
{
	Super::BeginPlay();
	
	CurrentTargetYaw = GetActorRotation().Yaw;
	
	if (HasAuthority()) 
	{
		CollisionBox->OnComponentBeginOverlap.AddDynamic(this, &ADC_Pawn::OnOverlapBegin);
	}
	
	SpawnNewSegment();
}

void ADC_Pawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

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
	
	// Lógica del árbitro (Snake) - SOLO SERVIDOR
	if (HasAuthority())
	{
		if (bIsTrailFinite && ActiveSegments.Num() > 0)
		{
			if (ADC_GameState* GS = GetWorld()->GetGameState<ADC_GameState>())
			{
				if (GS->bGlobalIsTrailFinite && ActiveSegments.Num() > 0)
				{
					float TotalLength = 0.f;
					for (ADC_TrailSegment* Seg : ActiveSegments)
					{
						if (Seg) TotalLength += FVector::Distance(Seg->StartLoc, Seg->EndLoc);
					}

					while (TotalLength > GS->GlobalMaxTrailLength && ActiveSegments.Num() > 0)
					{
						ADC_TrailSegment* OldestSeg = ActiveSegments[0];
						
						if (!OldestSeg)
						{
							ActiveSegments.RemoveAt(0);
							continue;
						}

						float Excess = TotalLength - GS->GlobalMaxTrailLength;
						float OldestLen = FVector::Distance(OldestSeg->StartLoc, OldestSeg->EndLoc);

						if (Excess >= OldestLen && ActiveSegments.Num() > 1)
						{
							TotalLength -= OldestLen;
							OldestSeg->Destroy();
							ActiveSegments.RemoveAt(0);
						}
						else
						{
							FVector Dir = (OldestSeg->EndLoc - OldestSeg->StartLoc).GetSafeNormal();
							FVector NewStart = OldestSeg->StartLoc + (Dir * Excess);
							OldestSeg->UpdateSegment(NewStart, OldestSeg->EndLoc);
							break;
						}
					}
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
	SpawnNewSegment();
}

void ADC_Pawn::SpawnNewSegment()
{
	if (HasAuthority() && TrailClass)
	{
		LastTurnLocation = GetActorLocation();
		FTransform SpawnTransform(FRotator::ZeroRotator, LastTurnLocation);
		
		ADC_TrailSegment* NewSegment = GetWorld()->SpawnActorDeferred<ADC_TrailSegment>(TrailClass, SpawnTransform, this);
		
		if (NewSegment)
		{
			// Congelamos la pared vieja si existe
			if (CurrentSegment)
			{
				CurrentSegment->bIsGrowing = false;
			}

			CurrentSegment = NewSegment;
			NewSegment->StartLoc = LastTurnLocation;
			NewSegment->EndLoc = LastTurnLocation;
			
			// Le asignamos el dueño y activamos el crecimiento local
			NewSegment->TargetPawn = this;
			NewSegment->bIsGrowing = true; 
			
			ActiveSegments.Add(NewSegment);
			NewSegment->FinishSpawning(SpawnTransform);
		}
	}
}

void ADC_Pawn::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (bIsDead) return;

	if (OtherActor && OtherActor != this && OtherActor != CurrentSegment)
	{
		if (OtherActor->IsA(ADC_TrailSegment::StaticClass()))
		{
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