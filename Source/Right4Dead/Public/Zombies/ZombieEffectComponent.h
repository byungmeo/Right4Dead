// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ZombieEffectComponent.generated.h"


class AZombieBase;

UCLASS(ClassGroup=(Custom), Blueprintable, meta=(BlueprintSpawnableComponent))
class RIGHT4DEAD_API UZombieEffectComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UZombieEffectComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Debugging")
	TObjectPtr<AZombieBase> Owner = nullptr;
	
	UFUNCTION(BlueprintImplementableEvent)
	void OnBulletHit(const FHitResult& HitInfo);
	UFUNCTION(BlueprintImplementableEvent)
	void OnMeleeHit(const FVector& BoneLocation);
	UFUNCTION(BlueprintImplementableEvent)
	void OnDead();
};
