// Fill out your copyright notice in the Description page of Project Settings.


#include "WhitekerBell.h"


// Sets default values
AWhitekerBell::AWhitekerBell()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
	Bell = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Bell"));
	Bell->SetupAttachment(RootComponent);

	//충돌체 설정
	Bell->SetGenerateOverlapEvents(true);

	Bell->SetCollisionProfileName(TEXT("WorldWeapon"));
}

// Called when the game starts or when spawned
void AWhitekerBell::BeginPlay()
{
	Super::BeginPlay();
}

void AWhitekerBell::Interaction()
{
	Super::Interaction();
	InteractionBell();
}

void AWhitekerBell::SetOverlayMaterial(UMaterialInterface* MyOverlayMaterial)
{
	Super::SetOverlayMaterial(MyOverlayMaterial);
	Bell->SetOverlayMaterial(MyOverlayMaterial);
}

void AWhitekerBell::ClearOverlayMaterial()
{
	Super::ClearOverlayMaterial();
	Bell->SetOverlayMaterial(nullptr);
}

