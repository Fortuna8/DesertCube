#include "DC_TrailSegment.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"

ADC_TrailSegment::ADC_TrailSegment()
{
	PrimaryActorTick.bCanEverTick = false; 
	bReplicates = true;

	RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	RootComponent = RootScene;

	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
	CollisionBox->SetupAttachment(RootComponent);
	CollisionBox->SetCollisionProfileName(TEXT("OverlapAllDynamic"));

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(RootComponent);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ADC_TrailSegment::UpdateSegment(FVector StartLocation, FVector EndLocation)
{
	// Guardamos las coordenadas internamente
	StartLoc = StartLocation;
	EndLoc = EndLocation;

	FVector Direction = (EndLocation - StartLocation).GetSafeNormal();
	float TotalDistance = FVector::Distance(StartLocation, EndLocation);

	// 1. Anclamos el segmento exactamente en la esquina donde doblaste
	SetActorLocation(StartLocation);
	SetActorRotation(Direction.Rotation());

	// 2. La malla visual (Mesh): El cubo básico mide 100. Al escalarlo, crece desde su centro.
	// Lo desplazamos la mitad de su largo hacia adelante para que su "espalda" quede anclada a la esquina.
	MeshComponent->SetRelativeLocation(FVector(TotalDistance / 2.0f, 0.f, 0.f));
	MeshComponent->SetWorldScale3D(FVector(TotalDistance / 100.0f, 0.2f, 1.0f)); 

	// 3. La colisión (Box): Le restamos 150 unidades de margen en la PUNTA delantera.
	float SafeDistance = FMath::Max(0.f, TotalDistance - 25.f);
	
	// Lo desplazamos también la mitad de SU largo para que su "espalda" siga pegada a la esquina.
	CollisionBox->SetRelativeLocation(FVector(SafeDistance / 2.0f, 0.f, 0.f));
	CollisionBox->SetBoxExtent(FVector(SafeDistance / 2.0f, 10.f, 50.f));
}