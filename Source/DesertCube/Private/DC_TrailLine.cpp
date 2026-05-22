#include "DC_TrailLine.h"
#include "DC_Pawn.h"
#include "Components/SplineComponent.h"
#include "Components/SplineMeshComponent.h"
#include "Net/UnrealNetwork.h"
#include "DC_GameState.h"

ADC_TrailLine::ADC_TrailLine()
{
	PrimaryActorTick.bCanEverTick = true; 
	bReplicates = true;
	bAlwaysRelevant = true; 

	SplineComp = CreateDefaultSubobject<USplineComponent>(TEXT("SplineComp"));
	RootComponent = SplineComp;
	SplineComp->ClearSplinePoints();
}

void ADC_TrailLine::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ADC_TrailLine, TargetPawn);
	DOREPLIFETIME(ADC_TrailLine, InitialPoint);
}

void ADC_TrailLine::BeginPlay()
{
	Super::BeginPlay();
}

void ADC_TrailLine::OnRep_InitialPoint()
{
	// Apenas el cliente recibe su punto de nacimiento asegurado, lo anota y arranca
	TurnCorners.Add(InitialPoint);
	bIsInitialized = true;
}

void ADC_TrailLine::Multicast_AddTurnPoint_Implementation(FVector NewPoint)
{
	// Cada computadora anota este punto en su propia memoria
	TurnCorners.Add(NewPoint);
	
	// (Opcional) Acá cumplís con el profe instanciando partículas o sonido en el futuro
}

void ADC_TrailLine::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Candado de seguridad de red
	if (!bIsInitialized || !TargetPawn || TurnCorners.Num() == 0) return;

	// 1. Agregamos temporalmente la cabeza (la posición actual de la moto) a 60 FPS
	TurnCorners.Add(TargetPawn->GetActorLocation());

	// 2. Recortamos la cola DE VERDAD (borramos los datos viejos localmente)
	if (ADC_GameState* GS = GetWorld()->GetGameState<ADC_GameState>())
	{
		if (GS->bGlobalIsTrailFinite)
		{
			float TotalLength = 0.f;
			for (int32 i = 0; i < TurnCorners.Num() - 1; i++)
			{
				TotalLength += FVector::Distance(TurnCorners[i], TurnCorners[i + 1]);
			}

			while (TotalLength > GS->GlobalMaxTrailLength && TurnCorners.Num() >= 2)
			{
				float Excess = TotalLength - GS->GlobalMaxTrailLength;
				float OldestLength = FVector::Distance(TurnCorners[0], TurnCorners[1]);

				if (Excess >= OldestLength && TurnCorners.Num() > 2)
				{
					TotalLength -= OldestLength;
					
					// Destruimos la malla física vieja para no acumular basura
					if (SplineMeshes.Num() > 0 && SplineMeshes[0])
					{
						SplineMeshes[0]->DestroyComponent();
						SplineMeshes.RemoveAt(0);
					}
					
					// Borramos el punto viejo para siempre
					TurnCorners.RemoveAt(0);
				}
				else
				{
					// Deslizamos el punto inicial
					FVector Dir = (TurnCorners[1] - TurnCorners[0]).GetSafeNormal();
					TurnCorners[0] += (Dir * Excess);
					break;
				}
			}
		}
	}

	// 3. Dibujamos la línea matemática afilada
	UpdateSplineMeshes(TurnCorners);

	// 4. Retiramos la cabeza temporal para que el próximo frame la vuelva a calcular bien
	TurnCorners.Pop();
}

void ADC_TrailLine::UpdateSplineMeshes(const TArray<FVector>& Points)
{
	if (Points.Num() < 2) return;

	int32 NeededMeshes = Points.Num() - 1;

	// A. Creamos mallas si faltan (ej. al doblar una esquina)
	while (SplineMeshes.Num() < NeededMeshes)
	{
		USplineMeshComponent* NewMesh = NewObject<USplineMeshComponent>(this);
		NewMesh->SetIsReplicated(false); 
		
		NewMesh->ComponentTags.Add(FName("SafeSegment"));
		
		NewMesh->SetStaticMesh(TrailMesh);
		if (TrailMaterial) NewMesh->SetMaterial(0, TrailMaterial);
		
		NewMesh->SetMobility(EComponentMobility::Movable);
		NewMesh->CreationMethod = EComponentCreationMethod::Instance;
		NewMesh->RegisterComponentWithWorld(GetWorld());
		NewMesh->AttachToComponent(SplineComp, FAttachmentTransformRules::KeepRelativeTransform);

		FVector2D WallThickness(0.2f, 1.0f);
		NewMesh->SetStartScale(WallThickness);
		NewMesh->SetEndScale(WallThickness);
		
		NewMesh->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
		NewMesh->SetGenerateOverlapEvents(true);

		SplineMeshes.Add(NewMesh);
	}

	// B. Actualizamos la línea matemática
	SplineComp->ClearSplinePoints();
	for (int32 i = 0; i < Points.Num(); i++)
	{
		SplineComp->AddSplinePoint(Points[i], ESplineCoordinateSpace::World, false);
		SplineComp->SetSplinePointType(i, ESplinePointType::Linear, true);
	}
	SplineComp->UpdateSpline();

	// C. Estiramos las mallas existentes y calculamos 90 grados perfectos
	for (int32 i = 0; i < SplineMeshes.Num(); i++)
	{
		FVector StartPos = SplineComp->GetLocationAtSplinePoint(i, ESplineCoordinateSpace::Local);
		FVector EndPos = SplineComp->GetLocationAtSplinePoint(i + 1, ESplineCoordinateSpace::Local);
		FVector Tangent = EndPos - StartPos;

		SplineMeshes[i]->SetStartAndEnd(StartPos, Tangent, EndPos, Tangent);

		// Inmunidad de colisión solo para la cabeza y el cuello
		if (i >= NeededMeshes - 2) {
			SplineMeshes[i]->ComponentTags.Add(FName("SafeSegment"));
		} else {
			SplineMeshes[i]->ComponentTags.Remove(FName("SafeSegment"));
		}
	}
}

bool ADC_TrailLine::IsSafeSegment(UPrimitiveComponent* Comp)
{
	if (Comp && Comp->ComponentHasTag(FName("SafeSegment"))) return true;
	return false;
}