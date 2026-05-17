// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DC_TrailSegment.generated.h"

UCLASS()
class DESERTCUBE_API ADC_TrailSegment : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ADC_TrailSegment();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
