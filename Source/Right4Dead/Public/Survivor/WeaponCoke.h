// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "WeaponBase.h"
#include "WeaponCoke.generated.h"

UCLASS()
class RIGHT4DEAD_API AWeaponCoke : public AWeaponBase
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AWeaponCoke();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
};
