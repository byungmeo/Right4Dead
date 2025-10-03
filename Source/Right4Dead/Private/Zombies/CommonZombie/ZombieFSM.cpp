#include "ZombieFSM.h"

#include "AIController.h"
#include "CommonZombie.h"
#include "Survivor.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Navigation/PathFollowingComponent.h"

UZombieFSM::UZombieFSM()
{
	PrimaryComponentTick.bCanEverTick = true;
	
	bAutoActivate = true;
}

void UZombieFSM::BeginPlay()
{
	Super::BeginPlay();
	Owner = Cast<ACommonZombie>(GetOwner());
}

void UZombieFSM::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	if (nullptr == Owner)
	{
		return;
	}

	if (nullptr != ChaseTarget)
	{
		// 추격 대상이 현재 유효하지 않다면 초기화 한다.
		if (false == IsValid(ChaseTarget))
		{
			ChaseTarget = nullptr;
		}
	}

	// 추격 대상이 존재한다면 추격 대상과의 거리를 갱신한다.
	if (ChaseTarget)
	{
		Distance = FVector::Dist(Owner->GetActorLocation(), ChaseTarget->GetActorLocation());	
	}

	// 현재 상태에 따라 Tick 함수를 호출한다.
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

// 현재 상태를 변경하는 메서드
void UZombieFSM::SetState(const EZombieState NewState)
{
	const EZombieState BeforeState = State;
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

	// 상태 변경을 알림
	OnChangedState.Broadcast(BeforeState, NewState);
}

#pragma region Idle
// 좀비가 대기 상태로 전환된경우 한 번 호출됩니다.
void UZombieFSM::StartIdle()
{
	// 실제로 움직임을 멈춘다.
	TriggerStopChase();
}

// 좀비가 대기 상태라면 계속 호출됩니다.
void UZombieFSM::TickIdle(const float DeltaTime)
{
	// 만약 추격하고 있는 대상이 생겼다면 추격 상태로 전환한다.
	if (ChaseTarget)
	{
		SetState(EZombieState::EZS_Chase);
		return;
	}

	// 일정 시간마다 주변에 플레이어가 있는지 탐색한다.
	CurrentIdleTime += DeltaTime;
	if (CurrentIdleTime > SearchInterval)
	{
		CurrentIdleTime = 0;
		if (bEnableIdleSearch)
		{
			// 구형태로 주변의 플레이어를 탐색
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

// 좀비가 대기 상태를 벗어난경우 한 번 호출됩니다.
void UZombieFSM::EndIdle()
{
	// 대기 시간을 초기화 한다.
	CurrentIdleTime = 0.0f;
}
#pragma endregion Idle

#pragma region Chase
// 좀비가 추격을 시작했을 때 한 번 호출됩니다.
void UZombieFSM::StartChase()
{
	TriggerStartChase(ChaseTarget);
}

// 좀비가 추격 중인 경우 계속 호출됩니다.
void UZombieFSM::TickChase(const float DeltaTime)
{
	// 만약 더 이상 추격 대상이 없다면 Idle 상태로 전환한다.
	if (nullptr == ChaseTarget)
	{
		SetState(EZombieState::EZS_Idle);
		return;
	}

	// 좀비가 NavMesh 영역에서 플레이어를 향해 달려가도록 한다.
	if (Owner->AIController && Owner->AIController->GetMoveStatus() == EPathFollowingStatus::Type::Idle)
	{
		TriggerStartChase(ChaseTarget);
	}

	// 만약, 좀비가 벽에 올라 타 있는 상태라면 대기 또는 공격 상태로 전환하지 않는다.
	if (true == Owner->bClimbing)
	{
		return;
	}

	// 만약, 최대 추격 지속 시간보다 오래 추격하고 있다면 추격을 중지한다.
	CurrentChaseTime += DeltaTime;
	if (CurrentChaseTime > StopChaseTime)
	{
		SetState(EZombieState::EZS_Idle);
		return;
	}

	// 공격 범위 내에 있으면 공격한다.
	if (Distance < NormalAttackRange)
	{
		SetState(EZombieState::EZS_Attack);
		return;
	}
	
	// 타겟이 인지 거리 내에 있으면 추적 지속 시간을 초기화 한다.
	if (Distance < Awareness)
	{
		CurrentChaseTime = 0.0f;
	}
}

// 좀비가 추격 상태에서 벗어난경우 한 번 호출됩니다.
void UZombieFSM::EndChase()
{
	// 추격 지속 시간을 초기화 환다.
	CurrentChaseTime = 0.0f;
}
#pragma endregion Chase

#pragma region Attack
// 좀비가 공격을 시작했을경우 한 번 호출됩니다.
void UZombieFSM::StartAttack()
{
	// 공격 쿨타임을 무시하고 즉시 공격합니다.
	CurrentAttackTime = NormalAttackInterval;
}

// 좀비가 공격하는동안 계속 호출됩니다.
void UZombieFSM::TickAttack(const float DeltaTime)
{
	// 만약 더 이상 추격 대상이 없다면 Idle 상태로 전환한다.
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

	// 현재 공격 쿨타임이 다 지난 상태인지 확인한 후 공격한다
	CurrentAttackTime += DeltaTime;
	if (CurrentAttackTime > NormalAttackInterval)
	{
		TriggerNormalAttack();
		OnNormalAttacked.Broadcast();
		CurrentAttackTime = 0.0f;
	}
}

// 좀비가 공격을 종료했을경우 한 번 호출됩니다.
void UZombieFSM::EndAttack()
{
	// 공격 쿨타임을 초기화 한다.
	CurrentAttackTime = 0.0f;
}
#pragma endregion Attack

#pragma region Dead
// 좀비가 죽음 상태가 되었을경우 한 번 호출됩니다.
void UZombieFSM::StartDead()
{
	ChaseTarget = nullptr;
}

// 좀비가 죽어있는동안 호출됩니다.
void UZombieFSM::TickDead(const float DeltaTime)
{
	Super::TickDead(DeltaTime);
}

// 좀비가 죽음 상태에서 벗어났을경우 한 번 호출됩니다.
void UZombieFSM::EndDead()
{
	Super::EndDead();
}
#pragma endregion Dead

void UZombieFSM::HandleDie()
{
	SetState(EZombieState::EZS_Dead);
	Owner->HandleDie();
}

void UZombieFSM::HandlePipeBombBeep(AActor* PipeBombActor)
{
	// 이미 파이프 폭탄을 향해 달려가고 있다면 무시한다.
	if (ChaseTarget == PipeBombActor)
	{
		return;
	}

	// 현재 플레이어를 공격 중이지 않다면 행동을 멈추고 추격 대상을 파이프 폭탄으로 변경한다.
	if (EZombieState::EZS_Attack != State)
	{
		SetState(EZombieState::EZS_Idle);
		ChaseTarget = PipeBombActor;
	}
}