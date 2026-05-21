#include "DC_TrailSegment.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Net/UnrealNetwork.h" // Obligatorio para replicar variables

ADC_TrailSegment::ADC_TrailSegment()
{
	// Fundamental: Ahora la estela sí necesita Tick
	PrimaryActorTick.bCanEverTick = true; 
	bReplicates = true;
	SetReplicatingMovement(true); 

	NetUpdateFrequency = 120.0f;
	MinNetUpdateFrequency = 60.0f;
	
	RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	RootComponent = RootScene;

	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
	CollisionBox->SetupAttachment(RootComponent);
	CollisionBox->SetCollisionProfileName(TEXT("OverlapAllDynamic"));

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(RootComponent);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	bIsGrowing = false;
	TargetPawn = nullptr;
}

void ADC_TrailSegment::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(ADC_TrailSegment, StartLoc);
	DOREPLIFETIME(ADC_TrailSegment, EndLoc);
	DOREPLIFETIME(ADC_TrailSegment, bIsGrowing);
	DOREPLIFETIME(ADC_TrailSegment, TargetPawn);
}

void ADC_TrailSegment::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// MAGIA VISUAL: Si está activa y tiene dueño, se estira sola en la PC de cada cliente
	if (bIsGrowing && TargetPawn)
	{
		UpdateSegment(StartLoc, TargetPawn->GetActorLocation());
	}
}

void ADC_TrailSegment::UpdateSegment(FVector StartLocation, FVector EndLocation)
{
	StartLoc = StartLocation;
	EndLoc = EndLocation;

	FVector Direction = (EndLocation - StartLocation).GetSafeNormal();
	float TotalDistance = FVector::Distance(StartLocation, EndLocation);

	SetActorLocation(StartLocation);
	SetActorRotation(Direction.Rotation());

	MeshComponent->SetRelativeLocation(FVector(TotalDistance / 2.0f, 0.f, 0.f));
	MeshComponent->SetWorldScale3D(FVector(TotalDistance / 100.0f, 0.2f, 1.0f)); 

	float SafeDistance = FMath::Max(0.f, TotalDistance - 25.f);
	
	CollisionBox->SetRelativeLocation(FVector(SafeDistance / 2.0f, 0.f, 0.f));
	CollisionBox->SetBoxExtent(FVector(SafeDistance / 2.0f, 10.f, 50.f));
}