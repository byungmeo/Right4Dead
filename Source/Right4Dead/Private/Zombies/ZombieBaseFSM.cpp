#include "ZombieBaseFSM.h"

#include "AIController.h"
#include "Survivor.h"
#include "ZombieBase.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Navigation/PathFollowingComponent.h"
#include "Right4Dead/Right4Dead.h"

UZombieBaseFSM::UZombieBaseFSM()
{
	PrimaryComponentTick.bCanEverTick = true;

	bAutoActivate = true;
}

void UZombieBaseFSM::BeginPlay()
{
	Super::BeginPlay();
	
	// Verbose 세팅은 인스턴스에서만 껏다 켯다 할 수 있도록 함
	bVerboseSearch = false;
}

void UZombieBaseFSM::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	if (nullptr == Owner)
	{
		return;
	}

	if (nullptr != ChaseTarget)
	{
		if (false==IsValid(ChaseTarget))
		{
			ChaseTarget = nullptr;
		}
	}
	
	if (ChaseTarget)
	{
		Distance = FVector::Dist(Owner->GetActorLocation(), ChaseTarget->GetActorLocation());	
	}
	
	switch (State)
	{
	case EZombieState::EZS_Idle:
		TickIdle(DeltaTime);
		break;
	case EZombieState::EZS_Chase:
		TickChase(DeltaTime);
		break;
	case EZombieState::EZS_Attack:
		TickAttack(DeltaTime);
		break;
	case EZombieState::EZS_Dead:
		TickDead(DeltaTime);
		break;
	}
}
void UZombieBaseFSM::SetState(const EZombieState NewState)
{
	const EZombieState& BeforeState = State;
	// 기존 상태에 따라 정리 작업
	switch (BeforeState)
	{
	case EZombieState::EZS_Idle:
		EndIdle();
		break;
	case EZombieState::EZS_Chase:
		EndChase();
		break;
	case EZombieState::EZS_Attack:
		EndAttack();
		break;
	case EZombieState::EZS_Dead:
		EndDead();
		break;
	}

	// 새로운 상태에 따른 초기화 작업
	switch (NewState)
	{
	case EZombieState::EZS_Idle:
		StartIdle();
		break;
	case EZombieState::EZS_Chase:
		StartChase();
		break;
	case EZombieState::EZS_Attack:
		StartAttack();
		break;
	case EZombieState::EZS_Dead:
		StartDead();
		break;
	}
	State = NewState;

	OnChangedState.Broadcast(BeforeState, NewState);
}

#pragma region Idle
void UZombieBaseFSM::StartIdle()
{
	TriggerStopChase();
}
void UZombieBaseFSM::TickIdle(const float DeltaTime)
{
	if (ChaseTarget)
	{
		SetState(EZombieState::EZS_Chase);
		return;
	}
	
	CurrentIdleTime += DeltaTime;
	if (CurrentIdleTime > SearchInterval)
	{
		CurrentIdleTime = 0;
		if (bEnableIdleSearch)
		{
			TArray<AActor*> ActorsToIgnore;
			ActorsToIgnore.Add(Owner); // 자기 자신은 검사에서 제외
			TArray<FHitResult> OutHits;
			const bool bHit = UKismetSystemLibrary::SphereTraceMulti(
				GetWorld(),
				Owner->GetActorLocation(),
				Owner->GetActorLocation(),
				Awareness,
				UEngineTypes::ConvertToTraceType(ECC_Pawn),
				false,
				ActorsToIgnore,
				(bVerboseSearch) ? EDrawDebugTrace::ForDuration : EDrawDebugTrace::None,
				OutHits,
				true
			);
			if (bHit && OutHits.Num() > 0)
			{
				for (auto HitResult : OutHits)
				{
					// TODO: Collision Channel이 정리되면 PipeBomb, 담즙 등에도 반응하도록 수정
					if (ASurvivor* Survivor = Cast<ASurvivor>(HitResult.GetActor()))
					{
						ChaseTarget = Survivor;
					}
				}
			}
		}
	}
}
void UZombieBaseFSM::EndIdle()
{
	CurrentIdleTime = 0.0f;
}
#pragma endregion Idle

#pragma region Chase
void UZombieBaseFSM::StartChase()
{
	TriggerStartChase(ChaseTarget);
}
void UZombieBaseFSM::TickChase(const float DeltaTime)
{
	if (nullptr == ChaseTarget)
	{
		SetState(EZombieState::EZS_Idle);
		return;
	}

	if (Owner->AIController && Owner->AIController->GetMoveStatus() == EPathFollowingStatus::Type::Idle)
	{
		TriggerStartChase(ChaseTarget);
	}
}
void UZombieBaseFSM::EndChase()
{
	CurrentChaseTime = 0.0f;
}
void UZombieBaseFSM::TriggerStartChase(const TObjectPtr<AActor>& Target)
{
	Owner->HandleStartChase(Target);
}
void UZombieBaseFSM::TriggerStopChase()
{
	Owner->HandleStopChase();
}
#pragma endregion Chase

#pragma region Attack
void UZombieBaseFSM::StartAttack()
{
	CurrentAttackTime = NormalAttackInterval;
}
void UZombieBaseFSM::TickAttack(const float DeltaTime)
{
	if (nullptr == ChaseTarget)
	{
		SetState(EZombieState::EZS_Idle);
		return;
	}

	// 공격 범위 안에 타겟이 없으면 추격 상태로 전환한다.
	if (Distance > NormalAttackRange)
	{
		SetState(EZombieState::EZS_Chase);
		return;
	}
	
	CurrentAttackTime += DeltaTime;
	if (CurrentAttackTime > NormalAttackInterval)
	{
		TriggerNormalAttack();
		OnNormalAttacked.Broadcast();
		CurrentAttackTime = 0.0f;
		return;
	}
}
void UZombieBaseFSM::EndAttack()
{
	CurrentAttackTime = 0.0f;
}
void UZombieBaseFSM::TriggerNormalAttack()
{
	Owner->HandleNormalAttack();
}
void UZombieBaseFSM::TriggerSpecialAttack()
{
	Owner->HandleSpecialAttack();
}
#pragma endregion Attack

#pragma region Dead
void UZombieBaseFSM::StartDead()
{
	ChaseTarget = nullptr;
}
void UZombieBaseFSM::TickDead(const float DeltaTime)
{
}
void UZombieBaseFSM::EndDead()
{
}
#pragma endregion Dead

void UZombieBaseFSM::HandleShove(const FVector& FromLocation)
{
	// ...
}
void UZombieBaseFSM::HandleDamage()
{
	// ...
}
void UZombieBaseFSM::HandleDie()
{
	SetState(EZombieState::EZS_Dead);
	Owner->HandleDie();
}