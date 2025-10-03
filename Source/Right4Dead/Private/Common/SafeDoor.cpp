// Fill out your copyright notice in the Description page of Project Settings.


#include "SafeDoor.h"


// Sets default values
ASafeDoor::ASafeDoor()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
	Root->SetRelativeScale3D(FVector(0.8f));
	
	Frame = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Frame"));
	const ConstructorHelpers::FObjectFinder<UStaticMesh> FrameMesh(TEXT("/Script/Engine.StaticMesh'/Game/Assets/Environments/SafeDoor/prison_door_2_2.prison_door_2_2'"));
	if (FrameMesh.Succeeded())
	{
		Frame->SetStaticMesh(FrameMesh.Object);
	}
	Frame->SetupAttachment(GetRootComponent());

	Hinge = CreateDefaultSubobject<USceneComponent>(TEXT("Hinge"));
	Hinge->SetupAttachment(GetRootComponent());
	Hinge->SetRelativeLocation(FVector(0, -70, 0));
	Hinge->SetRelativeRotation(FRotator(0, 90, 0));

	Door = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Door"));
	const ConstructorHelpers::FObjectFinder<UStaticMesh> DoorMesh(TEXT("/Script/Engine.StaticMesh'/Game/Assets/Environments/SafeDoor/prison_door_2_3.prison_door_2_3'"));
	if (DoorMesh.Succeeded())
	{
		Door->SetStaticMesh(DoorMesh.Object);
	}
	Door->SetupAttachment(Hinge);
	Door->SetRelativeLocation(FVector(0, 70, 0));

	Door->SetCollisionProfileName(TEXT("WorldWeapon"));
}

// Called when the game starts or when spawned
void ASafeDoor::BeginPlay()
{
	Super::BeginPlay();
}

void ASafeDoor::Interaction()
{
	Super::Interaction();
	InteractionDoor();
}

void ASafeDoor::SetOverlayMaterial(UMaterialInterface* MyOverlayMaterial)
{
	// 아무것도 하지 않음
}

void ASafeDoor::ClearOverlayMaterial()
{
	// 아무것도 하지 않음
}
