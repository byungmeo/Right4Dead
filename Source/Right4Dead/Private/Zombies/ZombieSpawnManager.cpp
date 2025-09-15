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

	// 좀비가 스폰되면 추적 대상인 플레이어를 캐싱한다
	InitTarget = UGameplayStatics::GetActorOfClass(GetWorld(), ASurvivor::StaticClass());

	// 레벨에 존재하는 좀비 스폰지점을 등록한다.
	TArray<AActor*> Actors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AZombieSpawnPoint::StaticClass(), Actors);
	for (auto* Actor : Actors)
	{
		SpawnPoints.Add(Cast<AZombieSpawnPoint>(Actor));
	}

	// 최대 좀비 수만큼 활성화 좀비 집합의 메모리 공간을 예약한다
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

	// 최대 스폰 가능한 좀비의 수 만큼 좀비를 새로 생성한 뒤 리스폰 대기열에 넣는다.
	for (int i = 0; i < MaxZombieCount - ActiveZombies.Num(); i++)
	{
		// 스폰에 실패하지 않도록 한다.
		FActorSpawnParameters SpawnParams;
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		// 플레이어 눈에 보이지 않는 장소에 좀비를 스폰시켜둔다.
		auto* Zombie = GetWorld()->SpawnActor<ACommonZombie>(ZombieFactory, FVector(-2000, 2000, 100), FRotator::ZeroRotator, SpawnParams);
		Zombie->SpawnManager = this;
		ActiveZombies.Add(Zombie);
		EnqueueZombie(Zombie);
	}
}

// 좀비를 리스폰 대기열(Queue)에 넣는 메서드
void AZombieSpawnManager::EnqueueZombie(ACommonZombie* Zombie)
{
	if (Zombie)
	{
		// 좀비가 죽은 경우 각종 기능을 비활성화
		{
			// 틱 중단
			Zombie->SetActorTickEnabled(false);
			Zombie->ZombieFSM->SetComponentTickEnabled(false);
			// 애니메이션 업데이트를 중단
			Zombie->ZombieAnimInstance->EnableUpdateAnimation(false);
			// AI Controller의 군중 회피 기능 비활성화
			Zombie->GetCharacterMovement()->bUseRVOAvoidance = false;
		}
		
		// 좀비를 리스폰 대기열에 넣고 활성화 집합에서 뺀다
		PoolCount++;
		ZombiePool.Enqueue(Zombie);
		ensureMsgf(ActiveZombies.Contains(Zombie), TEXT("활동 중인 좀비 집합에 해당 좀비가 없습니다."));
		ActiveZombies.Remove(Zombie);
	}
}

// 좀비를 리스폰 대기열(Queue)에서 빼내는 메서드
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

// 좀비 무리를 스폰하는 메서드 (좀비가 동적으로 스폰되는 유일한 경우의 수)
void AZombieSpawnManager::CallHorde()
{
	// 시뮬레이트 또는 게임 실행 중에만 동작
	const UWorld* World = GetWorld();
	if (nullptr == World || (World->WorldType != EWorldType::PIE && World->WorldType == EWorldType::Game))
	{
		return;
	}

	// 좀비 무리가 웅성이는 사운드 출력
	UGameplayStatics::PlaySound2D(this, HordeComingSound, 1, 1, -1);

	// 스폰 시켜야 할 좀비의 수
	int Rem = FMath::Min(NumOfHorde, PoolCount);
	// 다 스폰 시킬 때 까지 반복한다
	while (Rem > 0)
	{
		// 레벨 내의 스폰 지점들을 순회하며 고르게 하나씩 스폰시킨다
		for (const AZombieSpawnPoint* SpawnPoint : SpawnPoints)
		{
			// queue에서 좀비를 빼낸다
			if (--Rem < 0) break;
			ACommonZombie* Zombie = DequeueZombie();
			if (nullptr == Zombie) continue;

			// 좀비를 초기 상태로 복원한 뒤 스폰 위치로 이동시킨다
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
