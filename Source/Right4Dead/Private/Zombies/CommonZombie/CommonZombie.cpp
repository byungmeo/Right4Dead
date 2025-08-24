// Fill out your copyright notice in the Description page of Project Settings.


#include "CommonZombie.h"

#include "R4DHelper.h"
#include "Right4DeadGameInstance.h"
#include "Survivor.h"
#include "ZombieAudioComponent.h"
#include "ZombieFSM.h"
#include "ZombieSpawnManager.h"
#include "Engine/DamageEvents.h"
#include "Engine/StaticMeshActor.h"
#include "Kismet/GameplayStatics.h"
#include "Right4Dead/Right4Dead.h"

// Sets default values
ACommonZombie::ACommonZombie()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	static const ConstructorHelpers::FObjectFinder<USkeletalMesh> SkeletalMeshObj(TEXT("/Script/Engine.SkeletalMesh'/Game/Assets/ThirdPerson/Characters/Mannequin_UE4/Meshes/SK_Mannequin.SK_Mannequin'"));
	if (SkeletalMeshObj.Succeeded())
	{
		GetMesh()->SetSkeletalMeshAsset(SkeletalMeshObj.Object);
		GetMesh()->SetRelativeLocation(FVector(0, 0, -89));
		GetMesh()->SetRelativeRotation(FRotator(0, 270, 0));
		ConstructorHelpers::FClassFinder<UAnimInstance> AnimBlueprintClass(TEXT("/Script/Engine.AnimBlueprint'/Game/Blueprints/Zombies/CommonZombie/ABP_CommonZombie.ABP_CommonZombie_C'"));
        if (AnimBlueprintClass.Succeeded())
        {
        	GetMesh()->SetAnimInstanceClass(AnimBlueprintClass.Class);
        }
	}
	
	CommonZombieFSM = CreateDefaultSubobject<UZombieFSM>(TEXT("ZombieFSM"));
	ZombieFSM = CommonZombieFSM;
	static ConstructorHelpers::FClassFinder<UZombieAudioComponent> ZombieAudioComponentClass(TEXT("/Script/Engine.Blueprint'/Game/Blueprints/Zombies/CommonZombie/BP_CommonZombieAudioComponent.BP_CommonZombieAudioComponent_C'"));
	if (ZombieAudioComponentClass.Succeeded())
	{
		ZombieAudioComponentFactory = ZombieAudioComponentClass.Class;
	}
	
	static const ConstructorHelpers::FObjectFinder<UStaticMesh> HeadObj(TEXT("/Script/Engine.StaticMesh'/Game/Assets/Zombie/CommonZombie/Models/UE4MannequinDismemberment/SK_Mannequin_Head.SK_Mannequin_Head'"));
	static const ConstructorHelpers::FObjectFinder<UStaticMesh> ArmLeftObj(TEXT("/Script/Engine.StaticMesh'/Game/Assets/Zombie/CommonZombie/Models/UE4MannequinDismemberment/SK_Mannequin_Arm_Left.SK_Mannequin_Arm_Left'"));
	static const ConstructorHelpers::FObjectFinder<UStaticMesh> ArmRightObj(TEXT("/Script/Engine.StaticMesh'/Game/Assets/Zombie/CommonZombie/Models/UE4MannequinDismemberment/SK_Mannequin_Arm_Right.SK_Mannequin_Arm_Right'"));
	static const ConstructorHelpers::FObjectFinder<UStaticMesh> LegLeftObj(TEXT("/Script/Engine.StaticMesh'/Game/Assets/Zombie/CommonZombie/Models/UE4MannequinDismemberment/SK_Mannequin_Leg_Left.SK_Mannequin_Leg_Left'"));
	static const ConstructorHelpers::FObjectFinder<UStaticMesh> LegRightObj(TEXT("/Script/Engine.StaticMesh'/Game/Assets/Zombie/CommonZombie/Models/UE4MannequinDismemberment/SK_Mannequin_Leg_Right.SK_Mannequin_Leg_Right'"));
	if (HeadObj.Succeeded())
		HeadMesh = HeadObj.Object;
	if (ArmLeftObj.Succeeded())
		ArmLeftMesh = ArmLeftObj.Object;
	if (ArmRightObj.Succeeded())
		ArmRightMesh = ArmRightObj.Object;
	if (LegLeftObj.Succeeded())
		LegLeftMesh = LegLeftObj.Object;
	if (LegRightObj.Succeeded())
		LegRightMesh = LegRightObj.Object;
}

void ACommonZombie::BeginPlay()
{
	Super::BeginPlay();
	SetFolderPath(TEXT("Zombie/CommonZombie"));
}

void ACommonZombie::InitData()
{
	Hp = MaxHp = 50.0f;
	Speed = 250.0f;
	
	if (const URight4DeadGameInstance* GameInstance = Cast<URight4DeadGameInstance>(UGameplayStatics::GetGameInstance(this)))
	{
		switch (GameInstance->GetDifficulty())
		{
		case EDifficulty::Easy:
			TakeDamageMultiplier = 2; // 200%
			NormalAttackDamage = 1;
			break;
		case EDifficulty::Normal:
			TakeDamageMultiplier = 1; // 100%
			NormalAttackDamage = 2;
			break;
		case EDifficulty::Advanced:
			TakeDamageMultiplier = 0.75f; // 75%
			NormalAttackDamage = 5;
			break;
		case EDifficulty::Expert:
			TakeDamageMultiplier = 0.5f; // 50%
			NormalAttackDamage = 20;
			break;
		}
	}
}

AStaticMeshActor* SpawnPartMesh(USkeletalMeshComponent* SkeletalMesh, FName BoneName, UStaticMesh* SpawnMesh, FVector ImpulseDirection, float Power)
{
	// 이미 Bone이 숨겨진 상태라면 다시 스폰시키지 않는다
	if (SkeletalMesh->IsBoneHiddenByName(BoneName))
	{
		return nullptr;
	}
	// Bone을 숨겨라
	SkeletalMesh->HideBoneByName(BoneName, PBO_None);

	// Bone의 위치
	const FTransform Transform = SkeletalMesh->GetBoneTransform(BoneName);
	// 가짜 Static Mesh를 원래 Bone 위치에 스폰
	auto* StaticMeshActor = SkeletalMesh->GetWorld()->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), Transform);
	// 이동 가능하게
	StaticMeshActor->SetMobility(EComponentMobility::Type::Movable);
	StaticMeshActor->GetStaticMeshComponent()->SetCollisionProfileName(TEXT("DeadZombie"));
	// 물리 시뮬레이트 활성화
	StaticMeshActor->GetStaticMeshComponent()->SetSimulatePhysics(true);
	StaticMeshActor->GetStaticMeshComponent()->SetStaticMesh(SpawnMesh);
	// 적절한 방향으로 적절한 세기로 날린다
	ImpulseDirection *= Power;
	StaticMeshActor->GetStaticMeshComponent()->AddImpulse(ImpulseDirection);

	return StaticMeshActor;
}

void ACommonZombie::TriggerDismemberment(const FPointDamageEvent* PointDamageEvent) const
{
	PRINT_CALLINFO();
	const FName ParentBoneName = UR4DHelper::GetSignatureBone(GetMesh(), PointDamageEvent->HitInfo.BoneName);
	const FVector ShotDirection = PointDamageEvent->ShotDirection.GetSafeNormal();
	UE_LOG(LogTemp, Warning, TEXT("%s"), *ShotDirection.ToString());
	AStaticMeshActor* SpawnedMesh = nullptr; 
	if (ParentBoneName == TEXT("neck_01"))
	{
		SpawnedMesh = SpawnPartMesh(GetMesh(), TEXT("neck_01"), HeadMesh, ShotDirection, 1500);
	}
	else if (ParentBoneName == TEXT("lowerarm_l"))
	{
		SpawnedMesh = SpawnPartMesh(GetMesh(), TEXT("lowerarm_l"), ArmLeftMesh, ShotDirection, 1500);
	}
	else if (ParentBoneName == TEXT("lowerarm_r"))
	{
		SpawnedMesh = SpawnPartMesh(GetMesh(), TEXT("lowerarm_r"), ArmRightMesh, ShotDirection, 1500);
	}
	else if (ParentBoneName == TEXT("thigh_l"))
	{
		SpawnedMesh = SpawnPartMesh(GetMesh(), TEXT("thigh_l"), LegLeftMesh, ShotDirection, 1500);
	}
	else if (ParentBoneName == TEXT("thigh_r"))
	{
		SpawnedMesh = SpawnPartMesh(GetMesh(), TEXT("thigh_r"), LegRightMesh, ShotDirection, 1500);
	}
	if (SpawnedMesh)
	{
		OnDismemberment.Broadcast();
		FTimerHandle Handle;
		GetWorldTimerManager().SetTimer(Handle, [SpawnedMesh]()
		{
			SpawnedMesh->Destroy();
		}, 10.0f, false);
	}
}

void ACommonZombie::HandleDie()
{
	Super::HandleDie();
	FTimerHandle Handle;
	GetWorldTimerManager().SetTimer(Handle, [this]()
	{
		SpawnManager->EnqueueZombie(this);
	}, 2.0f, false);
}

void ACommonZombie::OnDie()
{
	Super::OnDie();
}

float ACommonZombie::TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent,
	class AController* EventInstigator, AActor* DamageCauser)
{
	const float Ret = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	// 죽었고, 죽은 원인이 PointDamage일 때만 해당 부위 사지분해 실시
	if (Hp <= 0)
	{
		if (const FPointDamageEvent* PointDamageEvent = (FPointDamageEvent*)&DamageEvent)
		{
			TriggerDismemberment(PointDamageEvent);
		}
	}

	return Ret;
}

void ACommonZombie::HandlePipeBombBeep(AActor* PipeBombActor) const
{
	CommonZombieFSM->HandlePipeBombBeep(PipeBombActor);
}
