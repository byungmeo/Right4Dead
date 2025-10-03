// Fill out your copyright notice in the Description page of Project Settings.


#include "CommonDoor.h"


// Sets default values
ACommonDoor::ACommonDoor()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
	
	Frame = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Frame"));
	Frame->SetupAttachment(GetRootComponent());

	Hinge = CreateDefaultSubobject<USceneComponent>(TEXT("Hinge"));
	Hinge->SetupAttachment(GetRootComponent());
	Hinge->SetRelativeLocation(FVector(10, -22.5, 0));

	Door = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Door"));
	Door->SetupAttachment(Hinge);
	Door->SetRelativeLocation(FVector(0, 4.75, 0));

	Door->SetCollisionProfileName(TEXT("WorldWeapon"));
}

// Called when the game starts or when spawned
void ACommonDoor::BeginPlay()
{
	Super::BeginPlay();
	
}

void ACommonDoor::Interaction()
{
	Super::Interaction();
	InteractionCommonDoor();
}

void ACommonDoor::SetOverlayMaterial(UMaterialInterface* MyOverlayMaterial)
{
	// 아무것도 하지 않음
}

void ACommonDoor::ClearOverlayMaterial()
{
	// 아무것도 하지 않음
}


