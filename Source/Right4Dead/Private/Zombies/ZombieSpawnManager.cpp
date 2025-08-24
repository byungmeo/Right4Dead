// Fill out your copyright notice in the Description page of Project Settings.


#include "ZombieSpawnManager.h"

#include "CommonZombie.h"
#include "Survivor.h"
#include "ZombieAnimInstance.h"
#include "ZombieBaseFSM.h"
#include "ZombieSpawnPoint.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"

// Sets default values
AZombieSpawnManager::AZombieSpawnManager()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	const ConstructorHelpers::FObjectFinder<USoundWave> SoundObj(TEXT("/Script/Engine.SoundWave'/Game/Assets/Sounds/Horde/HordeComingSound.HordeComingSound'"));
	if (SoundObj.Succeeded())
	{
		HordeComingSound = SoundObj.Object;
	}
	
	ConstructorHelpers::FClassFinder<ACommonZombie> ZombieClass(TEXT("/Script/Engine.Blueprint'/Game/Blueprints/Zombies/BP_CommonZombie.BP_CommonZombie_C'"));
    if (ZombieClass.Succeeded())
    {
    	ZombieFactory = ZombieClass.Class;
    }
}

// Called when the game starts or when spawned
void AZombieSpawnManager::BeginPlay()
{
	Super::BeginPlay();

	InitTarget = UGameplayStatics::GetActorOfClass(GetWorld(), ASurvivor::StaticClass());
	
	TArray<AActor*> Actors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AZombieSpawnPoint::StaticClass(), Actors);
	for (auto* Actor : Actors)
	{
		SpawnPoints.Add(Cast<AZombieSpawnPoint>(Actor));
	}

	ActiveZombies.Reserve(MaxZombieCount);

	// 미리 레벨에 배치되어있는 좀비들을 등록 
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACommonZombie::StaticClass(), Actors);
	for (auto* Actor : Actors)
	{
		auto* Zombie = Cast<ACommonZombie>(Actor);
		Zombie->SpawnManager = this;
		// Zombie->InitStart();
		ActiveZombies.Add(Cast<ACommonZombie>(Actor));
	}

	// 최대 스폰 가능한 좀비의 수 만큼 좀비를 새로 생성한 뒤 등록
	for (int i = 0; i < MaxZombieCount - ActiveZombies.Num(); i++)
	{
		FActorSpawnParameters SpawnParams;
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		auto* Zombie = GetWorld()->SpawnActor<ACommonZombie>(ZombieFactory, FVector(-2000, 2000, 100), FRotator::ZeroRotator, SpawnParams);
		Zombie->SpawnManager = this;
		ActiveZombies.Add(Zombie);
		EnqueueZombie(Zombie);
	}
}

void AZombieSpawnManager::EnqueueZombie(ACommonZombie* Zombie)
{
	if (Zombie)
	{
		if (false == ActiveZombies.Contains(Zombie))
		{
			return;
		}
		
		// 각종 기능 비활성화
		{
			Zombie->SetActorTickEnabled(false);
			Zombie->ZombieFSM->SetComponentTickEnabled(false);
			Zombie->ZombieAnimInstance->EnableUpdateAnimation(false);
			Zombie->GetCharacterMovement()->bUseRVOAvoidance = false;
		}
		
		// 좀비를 리스폰 대기열에 넣고 활성화 집합에서 뺀다
		PoolCount++;
		ZombiePool.Enqueue(Zombie);
		ActiveZombies.Remove(Zombie);
	}
}

ACommonZombie* AZombieSpawnManager::DequeueZombie()
{
	ACommonZombie* Zombie = nullptr;
	do
	{
		if (PoolCount == 0)
		{
			break;
		}
		PoolCount--;
		ZombiePool.Dequeue(Zombie);
	} while (nullptr == Zombie || ActiveZombies.Contains(Zombie));
	if (Zombie)
	{
		ActiveZombies.Add(Zombie);
	}
	return Zombie;
}

// Called every frame
void AZombieSpawnManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AZombieSpawnManager::CallHorde()
{
	// 시뮬레이트 또는 게임 실행 중에만 동작
	const UWorld* World = GetWorld();
	if (nullptr == World || (World->WorldType != EWorldType::PIE && World->WorldType == EWorldType::Game))
	{
		return;
	}
	
	UGameplayStatics::PlaySound2D(this, HordeComingSound, 1, 1, -1);
	
	int Rem = FMath::Min(NumOfHorde, PoolCount);
	while (Rem > 0)
	{
		for (const auto* SpawnPoint : SpawnPoints)
		{
			if (--Rem < 0) break;
			auto* Zombie = DequeueZombie();
			if (nullptr == Zombie) continue;

			Zombie->GetMesh()->SetSimulatePhysics(false);
            Zombie->GetMesh()->SetCollisionProfileName(TEXT("CharacterMesh"));
            Zombie->GetCapsuleComponent()->SetCollisionProfileName(TEXT("Pawn"));
			Zombie->GetMesh()->SetRelativeLocation(FVector(0, 0, -89));
			Zombie->GetMesh()->SetRelativeRotation(FRotator(0, -90, 0));
			Zombie->InitStart();
			Zombie->ZombieFSM->ChaseTarget = InitTarget;
			Zombie->SetActorLocation(SpawnPoint->GetActorLocation(), false, nullptr, ETeleportType::TeleportPhysics);
			Zombie->SetActorRotation(SpawnPoint->GetActorRotation(), ETeleportType::TeleportPhysics);
			
			Zombie->SetActorTickEnabled(true);
			Zombie->ZombieFSM->SetComponentTickEnabled(true);
			Zombie->ZombieAnimInstance->EnableUpdateAnimation(true);
			Zombie->GetCharacterMovement()->bUseRVOAvoidance = true;
		}
	}
}
