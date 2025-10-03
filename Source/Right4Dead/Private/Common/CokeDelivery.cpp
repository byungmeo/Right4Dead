// Fill out your copyright notice in the Description page of Project Settings.


#include "CokeDelivery.h"


// Sets default values
ACokeDelivery::ACokeDelivery()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
}

// Called when the game starts or when spawned
void ACokeDelivery::BeginPlay()
{
	Super::BeginPlay();
	
}

void ACokeDelivery::Interaction()
{
	Super::Interaction();
	InteractionDelivery();
}

void ACokeDelivery::SetOverlayMaterial(UMaterialInterface* MyOverlayMaterial)
{
	Super::SetOverlayMaterial(MyOverlayMaterial);
	bIsCokeDelivery = true;
	Door->SetOverlayMaterial(MyOverlayMaterial);
}

void ACokeDelivery::ClearOverlayMaterial()
{
	Super::ClearOverlayMaterial();
	bIsCokeDelivery = true;
	Door->SetOverlayMaterial(nullptr);
}

