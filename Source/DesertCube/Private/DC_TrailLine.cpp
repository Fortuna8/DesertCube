#include "DC_TrailLine.h"
#include "DC_Pawn.h"
#include "Net/UnrealNetwork.h"
#include "DC_GameState.h"

ADC_TrailLine::ADC_TrailLine()
{
	PrimaryActorTick.bCanEverTick = true; 
	bReplicates = true;
	bAlwaysRelevant = true; 
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
	TurnCorners.Add(InitialPoint);
	bIsInitialized = true;
}

void ADC_TrailLine::Multicast_AddTurnPoint_Implementation(FVector NewPoint)
{
	TurnCorners.Add(NewPoint);
}

void ADC_TrailLine::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bIsInitialized || !TargetPawn || TurnCorners.Num() == 0) return;

	TurnCorners.Add(TargetPawn->GetActorLocation()); // Cabeza viva

	// Lógica Pura: Recorte de cola sin mallas visuales
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
					TurnCorners.RemoveAt(0); // Borrado de memoria limpia
				}
				else
				{
					FVector Dir = (TurnCorners[1] - TurnCorners[0]).GetSafeNormal();
					TurnCorners[0] += (Dir * Excess);
					break;
				}
			}
		}
	}

	TurnCorners.Pop(); // Retiramos la cabeza temporal
}

// MAGIA PURA: Colisión Matemática 2D
bool ADC_TrailLine::CheckMathematicalCollision(FVector MoveStart, FVector MoveEnd, float BikeRadius, AActor* CheckingPawn)
{
	if (!bIsInitialized || TurnCorners.Num() == 0) return false;
	
	TArray<FVector> MathPoints = TurnCorners;
	if (TargetPawn) MathPoints.Add(TargetPawn->GetActorLocation());
	
	if (MathPoints.Num() < 2) return false;
	
	bool bIsOwnTrail = (CheckingPawn == TargetPawn);
	
	for (int32 i = 0; i < MathPoints.Num() - 1; i++)
	{
		// ZONA SEGURA: Ignoramos la cabeza y el cuello de nuestra propia estela
		if (bIsOwnTrail && i >= MathPoints.Num() - 3) continue;
		
		FVector PointA = MathPoints[i];
		FVector PointB = MathPoints[i+1];
		
		// 1. Verificamos si las líneas se cruzan formando una X o una T
		FVector Intersection;
		bool bCrosses = FMath::SegmentIntersection2D(MoveStart, MoveEnd, PointA, PointB, Intersection);
		
		// 2. Verificamos cercanía para simular el "Grosor" de la pared
		float DistanceToWall = FMath::PointDistToSegment(MoveEnd, PointA, PointB);
		
		if (bCrosses || DistanceToWall < BikeRadius)
		{
			return true; // Impacto confirmado
		}
	}
	return false;
}