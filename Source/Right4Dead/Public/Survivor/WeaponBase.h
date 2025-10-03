// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EWeaponType.h"
#include "FWeaponData.h"
#include "GameFramework/Actor.h"
#include "WeaponBase.generated.h"

UCLASS()
class RIGHT4DEAD_API AWeaponBase : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AWeaponBase();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	//외관
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	class USphereComponent* Root = nullptr;
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	class UStaticMeshComponent* PrimaryWeapon = nullptr;
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	class USceneComponent* Muzzle = nullptr;
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	class USceneComponent* Eject = nullptr;

	// 무기 데이터 구조체
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon Data")
	FWeaponData WeaponData;
	
	//Weapon 변수
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Weapon")
	EWeaponType SlotType;

	//SKYE: 월드에 소환된 무기(장착X)와 내가 장착하고 있는 무기를 구분하기 위한 설정 추가
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Weapon")
	bool IsEquipped = false;
	void SetEquipped(bool bEquip);

	//트레이싱 됐을때 overlay 재질 설정해주는 함수
	TArray<UMaterialInterface*> OriginalMaterials;
	void SetOverlayMaterial(UMaterialInterface* MyOverlayMaterial);
	void ClearOverlayMaterial();

	virtual void ThrownInteraction();
};


