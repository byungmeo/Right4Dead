// Fill out your copyright notice in the Description page of Project Settings.


#include "WeaponBase.h"
#include "Survivor.h"
#include "Camera/CameraComponent.h"
#include "Components/SphereComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"


// Sets default values
AWeaponBase::AWeaponBase()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	//외관
	Root=CreateDefaultSubobject<USphereComponent>(TEXT("Root"));
	SetRootComponent(Root);
	PrimaryWeapon=CreateDefaultSubobject<UStaticMeshComponent>("PrimaryWeapon");
	PrimaryWeapon->SetupAttachment(RootComponent);
	PrimaryWeapon->CastShadow = false;
	Muzzle=CreateDefaultSubobject<USceneComponent>("Muzzle");
	Muzzle->SetupAttachment(PrimaryWeapon);
	Muzzle->SetRelativeLocationAndRotation(FVector(0,57,-2.5),FRotator(0,90,0));
	Eject=CreateDefaultSubobject<USceneComponent>("Eject");
	Eject->SetupAttachment(Muzzle);
	Eject->SetRelativeLocation(FVector(-50,0,0));

	//충돌체 설정
	PrimaryWeapon->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Root->SetGenerateOverlapEvents(true);
	//SKYE: 무기 프리셋 변경3
	Root->SetCollisionProfileName(TEXT("WorldWeapon"));
	Root->SetCanEverAffectNavigation(false);

}

// Called when the game starts or when spawned
void AWeaponBase::BeginPlay()
{
	Super::BeginPlay();
	// 시작하면 총알수를 세팅해주고
	WeaponData.CurrentAmmo = WeaponData.MaxAmmo;
	SlotType = WeaponData.WeaponName;
}

void AWeaponBase::SetEquipped(bool bEquip)
{
	IsEquipped = bEquip;

	//월드에 있는 무기는 bIsEquipped를 false로 설정
	//무기를 장착하면 true로 설정을 변경해주자
}

void AWeaponBase::SetOverlayMaterial(UMaterialInterface* MyOverlayMaterial)
{
	PrimaryWeapon->SetOverlayMaterial(MyOverlayMaterial);
}

void AWeaponBase::ClearOverlayMaterial()
{
	PrimaryWeapon->SetOverlayMaterial(nullptr);
}

void AWeaponBase::ThrownInteraction()
{
}





