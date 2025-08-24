#include "ZombieBase.h"

#include "AIController.h"
#include "BulletDamageType.h"
#include "CommonZombieAIController.h"
#include "ExplosionDamageType.h"
#include "MeleeDamageType.h"
#include "R4DHelper.h"
#include "ShoveDamageType.h"
#include "Survivor.h"
#include "ZombieAnimInstance.h"
#include "ZombieAudioComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/DamageEvents.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "Right4Dead/Right4Dead.h"

AZombieBase::AZombieBase()
{
	PrimaryActorTick.bCanEverTick = true;
	// 좀비의 Tick 간격을 늘려 좀비가 대규모로 활성화되었을 때 부담을 줄임
	SetActorTickInterval(0.1f);
	/*
	 *	ObjectPooling을 통해 게임이 끝날 때 까지 GC에 의해 정리될 일이 없으므로
	 *	Cluster로 묶어 GC 도달 가능성 분석을 빠르게 개선할 수 있도록 활성화
	 */
	bCanBeInCluster = true;
	
	AIControllerClass = ACommonZombieAIController::StaticClass();
	FinalDamage = 0;
	Hp = MaxHp = 0;
	Speed = 0;
	TakeDamageMultiplier = 1;
	PartDamageMultipliers = {4, 1.25f, 0.75f, 1};
	bTakeDamaged = false;
	OnTakeAnyDamage.AddDynamic(this, &AZombieBase::OnTakeAnyDamageHandler);
	OnTakePointDamage.AddDynamic(this, &AZombieBase::OnTakePointDamageHandler);
	OnTakeRadialDamage.AddDynamic(this, &AZombieBase::OnTakeRadialDamageHandler);
	
	// TPS Character 기본 설정값
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->GravityScale = 1.75f;
	Movement->MaxAcceleration = 1500.0f;
	Movement->BrakingFrictionFactor = 1.0f;
	Movement->bUseSeparateBrakingFriction = true;
	Movement->MaxWalkSpeed = 500.0f;
	Movement->MinAnalogWalkSpeed = 20.0f;
	Movement->BrakingDecelerationWalking = 2000.0f;
	Movement->JumpZVelocity = 700.0f;
	Movement->BrakingDecelerationFalling = 1500.0f;
	Movement->AirControl = 0.35f;
	Movement->RotationRate = FRotator(0, 500, 0);
	Movement->bOrientRotationToMovement = true;
	Movement->bUseRVOAvoidance = true;
	Movement->AvoidanceConsiderationRadius = 160;
	Movement->SetFixedBrakingDistance(200.0f);
	Movement->GetNavMovementProperties()->bUseAccelerationForPaths = true;
	Movement->GetNavMovementProperties()->bUseFixedBrakingDistanceForPaths = true;
	Movement->GetNavAgentPropertiesRef().AgentRadius = 42.0f;
	Movement->GetNavAgentPropertiesRef().AgentHeight = 192.0f;
}

void AZombieBase::ForceDie()
{
	OnDie();
}

void AZombieBase::BeginPlay()
{
	Super::BeginPlay();
	InitData();
	ZombieAnimInstance = Cast<UZombieAnimInstance>(GetMesh()->GetAnimInstance());
	AIController = Cast<ACommonZombieAIController>(GetController());
	if (nullptr == AIController)
	{
		if (nullptr == (AIController = Cast<ACommonZombieAIController>(GetWorld()->SpawnActor(AIControllerClass))))
		{
			UE_LOG(LogTemp, Error, TEXT("Failed Set AI Controller"));
		}
		else
		{
			AIController->Possess(this);
		}
	}

	if (ZombieAudioComponentFactory->StaticClass())
	{
		ZombieAudioComponent = NewObject<UZombieAudioComponent>(this, ZombieAudioComponentFactory, TEXT("ZombieAudioComponent"), RF_Transactional);
        if(ZombieAudioComponent)
        {
        	ZombieAudioComponent->RegisterComponent();
        }
	}
}

void AZombieBase::InitStart()
{
	if (ZombieFSM)
	{
		ZombieFSM->SetState(EZombieState::EZS_Idle);
	}
	Hp = MaxHp;
	bTakeDamaged = false;
	// TODO: Bone 숨긴거 다시 표시
}

void AZombieBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bClimbing)
	{
		const FVector P0 = GetActorLocation();
		const bool bIsNearZ = GetCharacterMovement()->GetFeetLocation().Z >= ClimbDestination.GetLocation().Z;
		if (false == bIsNearZ) // 아직 덜 올라왔다면
		{
			// Z축으로만 오른다
			const FVector P = P0 + FVector(0, 0, 1) * Speed * DeltaSeconds;
			SetActorLocation(P);
		}
		else
		{
			if (FVector::Dist(GetCharacterMovement()->GetFeetLocation(), ClimbDestination.GetLocation()) > 1000)
			{
				EndClimbing();
				return;
			}
			const FVector P = P0 + GetActorForwardVector() * Speed * DeltaSeconds;
			SetActorLocation(P);
		}
	}
}

void AZombieBase::HandleDieFromExplosion(const FVector& ExplosionOrigin, const float& Radius) const
{
	GetMesh()->SetSimulatePhysics(true);
	GetMesh()->AddRadialImpulse(ExplosionOrigin, Radius, 60000, RIF_Linear);
}

float AZombieBase::TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent,
                              class AController* EventInstigator, AActor* DamageCauser)
{
	PRINT_CALLINFO();
	
	// 언리얼에서 제공하는 Damage 프레임워크 함수 호출 스택
	// Apply(..)Damage -> TakeDamage -> OnTake(...)DamageHandler
	//								 -> OnTakeAnyDamageHandler
	
	// 1. Super::TakeDamage 호출 전 피해 배율을 적용한다.
	// 2. Point 또는 RadialDamage Handler에서 부위별 데미지 또는 거리별 데미지를 계산하고 FinalDamage에 저장한다.
	// 3. OnDamage를 실행시키고 FinalDamage를 반환한다.
	
	DamageAmount *= TakeDamageMultiplier;
	FinalDamage = DamageAmount;
	Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	// FinalDamage can set on damage handler
	OnDamaged(FinalDamage);

	if (Hp > 0)
	{
		if (DamageEvent.DamageTypeClass == UShoveDamageType::StaticClass())
		{
			OnShove.Broadcast();
			HandleShove(DamageCauser->GetActorLocation());
		}
		else if (DamageEvent.DamageTypeClass == UBulletDamageType::StaticClass())
		{
			if (ZombieAnimInstance)
			{
				ZombieAnimInstance->PlayHit();
			}
		}
	}
	else
	{
		if (DamageEvent.DamageTypeClass == UExplosionDamageType::StaticClass())
		{
			const FRadialDamageEvent* RadialDamageEvent = (FRadialDamageEvent*) &DamageEvent;
			HandleDieFromExplosion(DamageCauser->GetActorLocation() + FVector(0, 0, -50), 1000);
			return FinalDamage;
		}
	}
	
	if (DamageEvent.DamageTypeClass == UBulletDamageType::StaticClass())
	{
		const FPointDamageEvent* PointDamageEvent = (FPointDamageEvent*) &DamageEvent;
		OnBulletHit.Broadcast(PointDamageEvent->HitInfo);
	}
	if (DamageEvent.DamageTypeClass == UMeleeDamageType::StaticClass())
	{
		const FPointDamageEvent* PointDamageEvent = (FPointDamageEvent*) &DamageEvent;
		OnMeleeHit.Broadcast(GetMesh()->GetBoneLocation(PointDamageEvent->HitInfo.BoneName));
	}
	
	return FinalDamage;
}

void AZombieBase::OnTakeAnyDamageHandler(AActor* DamagedActor, float Damage, const class UDamageType* DamageType,
	class AController* InstigatedBy, AActor* DamageCauser)
{
	PRINT_CALLINFO();
	// OnTakeAnyDamageHandler는 DamageEvent가 Point 또는 Radial이라도 마지막에 반드시 호출된다.
}

void AZombieBase::OnTakePointDamageHandler(AActor* DamagedActor, float Damage, class AController* InstigatedBy,
	FVector HitLocation, class UPrimitiveComponent* FHitComponent, FName BoneName, FVector ShotFromDirection,
	const class UDamageType* DamageType, AActor* DamageCauser)
{
	const FName ParentBoneName = UR4DHelper::GetSignatureBone(GetMesh(), BoneName);
	if (ParentBoneName == TEXT("neck_01"))
	{
		Damage *= PartDamageMultipliers.Head;
	}
	else if (ParentBoneName == TEXT("lowerarm_l") || ParentBoneName == TEXT("lowerarm_r") || ParentBoneName == TEXT("thigh_l") || ParentBoneName == TEXT("thigh_r"))
	{
		Damage *= PartDamageMultipliers.Legs;
	}
	else if (ParentBoneName == TEXT("spine_02"))
	{
		Damage *= PartDamageMultipliers.Thorax;
	}
	else
	{
		Damage *= PartDamageMultipliers.Stomach;
	}
	FinalDamage = Damage;
}

void AZombieBase::OnTakeRadialDamageHandler(AActor* DamagedActor, float Damage, const class UDamageType* DamageType,
	FVector Origin, const FHitResult& HitInfo, class AController* InstigatedBy, AActor* DamageCauser)
{
	PRINT_CALLINFO();
	FinalDamage = Damage;
}

void AZombieBase::HandleDie()
{
	GetMesh()->SetCollisionProfileName(TEXT("DeadZombie"));
	GetCapsuleComponent()->SetCollisionProfileName(TEXT("DeadZombie"));
	// SetActorEnableCollision(false);
}

void AZombieBase::HandleNormalAttack()
{
	ZombieAnimInstance->PlayAttack();
	if (IActorBase* Actor = Cast<IActorBase>(ZombieFSM->ChaseTarget))
	{
		Actor->OnDamaged(NormalAttackDamage);
	}
}

void AZombieBase::HandleSpecialAttack()
{
}

void AZombieBase::HandleShove(const FVector& FromLocation)
{
	ZombieFSM->HandleShove(FromLocation);
	
	if (ZombieAnimInstance)
	{
		
		const FVector LocationA = Owner->GetActorLocation();
		const FVector ForwardVectorA = Owner->GetActorForwardVector();
		
		// 상대 액터의 Z축 좌표는 좀비와 동일하게 맞춘다.
		const FVector LocationB =  FVector(FromLocation.X, FromLocation.Y, LocationA.Z);
		
		// 플레이어와 좀비의 위치 벡터를 뺄셈 계산하여 플레이어 기준에서 좀비쪽으로 가리키는 방향 벡터를 구한다.
		const FVector DirVector = (LocationB - LocationA).GetSafeNormal();
		
		// 내적으로 위아래를 구분할 수 있게 하고
		double Theta = FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(ForwardVectorA, DirVector)));
		
		// 외적으로 좌우를 구분할 수 있게 한다. (Z값이 0 미만이면 Theta의 부호를 반대로 하기만 하면 된다)
		const FVector CrossProduct = FVector::CrossProduct(ForwardVectorA, DirVector);
		if (CrossProduct.Z < 0)
		{
			Theta *= -1.f;
		}
		// AnimBlueprint에서 Theta값에 따른 밀치기 피격 애니메이션을 재생하자.
		// -45 ~ 45 : 좀비가 정면에서 밀치기를 맞았다 (뒤로 밀리는 애니메이션)
		//  45 ~ 135 : 좀비가 오른쪽에서 밀치기를 맞았다 (왼쪽으로 밀리는 애니메이션)
		// -180 ~ -135 / 135 ~ 180 : 좀비가 뒤에서 밀치기를 맞았다 (앞으로 밀리는 애니메이션)
		// -135 ~ -45 : 좀비가 왼쪽에서 밀치기를 맞았다 (오른쪽으로 밀리는 애니메이션)
		// 추후 C++에서 분기를 나눌 예정
		ZombieAnimInstance->PlayKnockBack(Theta);
	}
}

void AZombieBase::HandleStartChase(const TObjectPtr<AActor>& Target) const
{
	if (AIController && AIController->GetMoveStatus() == EPathFollowingStatus::Type::Idle)
	{
		AIController->MoveToActor(Target);
	}
}

void AZombieBase::HandleStopChase() const
{
	if (AIController && AIController->GetMoveStatus() == EPathFollowingStatus::Moving)
	{
		AIController->StopMovement();
	}
}


void AZombieBase::OnDamaged(const float Damage)
{
	const float OrgHp = Hp;
	Hp -= Damage;
	UE_LOG(LogTemp, Display, TEXT("HP %f -> %f"), OrgHp, Hp);
	if (Hp < 0)
	{
		OnDie();
	}
}

void AZombieBase::OnDie()
{
	PRINT_CALLINFO();
	ZombieFSM->HandleDie();
	OnDead.Broadcast();
}

void AZombieBase::StartClimbing(const FTransform& Destination)
{
	AIController->GetPathFollowingComponent()->PauseMove();
	GetCharacterMovement()->SetMovementMode(MOVE_Flying);
	bClimbing = true;
	ClimbDestination = Destination;
	SetActorRotation(Destination.Rotator());
}

void AZombieBase::EndClimbing()
{
	if (false == bClimbing)
	{
		return;
	}
	bClimbing = false;
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	AIController->GetPathFollowingComponent()->ResumeMove();
	ClimbDestination = FTransform::Identity;
}

EZombieState AZombieBase::GetState() const
{
	return ZombieFSM->State;
}
