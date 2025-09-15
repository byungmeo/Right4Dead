// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ZombieSpawnManager.generated.h"

class ACommonZombieAIController;
class ACommonZombie;
class AZombieSpawnPoint;
class ASurvivor;

UCLASS()
class RIGHT4DEAD_API AZombieSpawnManager : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AZombieSpawnManager();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	
	UPROPERTY(VisibleInstanceOnly, Category="Debugging")
	TArray<AZombieSpawnPoint*> SpawnPoints;

	TSubclassOf<ACommonZombie> ZombieFactory;

	UPROPERTY(VisibleInstanceOnly, Category="Debugging")
	TObjectPtr<AActor> InitTarget = nullptr;
	
	UPROPERTY()
	TObjectPtr<USoundWave> HordeComingSound = nullptr;

	UPROPERTY(EditAnywhere, Category="Debugging")
	int NumOfHorde = 30;
	UFUNCTION(CallInEditor, BlueprintCallable, Category="Debugging")
	void CallHorde();
	
	void EnqueueZombie(ACommonZombie* Zombie);
	
	ACommonZombie* DequeueZombie();
	
	UPROPERTY(EditAnywhere, Category="Debugging")
	int MaxZombieCount = 90;
	
	UPROPERTY(VisibleInstanceOnly, Category="Debugging")
	TSet<ACommonZombie*> ActiveZombies;
private:
	UPROPERTY(VisibleInstanceOnly, Category="Debugging")
	int PoolCount = 0;
	
	TQueue<ACommonZombie*, EQueueMode::SingleThreaded> ZombiePool;
};
