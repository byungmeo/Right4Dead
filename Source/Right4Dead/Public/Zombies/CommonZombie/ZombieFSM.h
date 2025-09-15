#pragma once

#include "CoreMinimal.h"
#include "ZombieBaseFSM.h"
#include "Components/ActorComponent.h"
#include "ZombieFSM.generated.h"

class AAIController;
class UZombieAnimInstance;
class UCharacterMovementComponent;
class ACommonZombie;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class RIGHT4DEAD_API UZombieFSM : public UZombieBaseFSM
{
	GENERATED_BODY()

public:
	UZombieFSM();

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;

	virtual void SetState(const EZombieState NewState);
	
	/*
	 *Idle
	 */
	virtual void StartIdle() override;
	virtual void TickIdle(const float DeltaTime) override;
	virtual void EndIdle() override;

	/*
	 *Chase
	 */
	virtual void StartChase() override;
	virtual void TickChase(const float DeltaTime) override;
	virtual void EndChase() override;

	/*
	 *Attack
	 */
	virtual void StartAttack() override;
	virtual void TickAttack(const float DeltaTime) override;
	virtual void EndAttack() override;

	/*
	 *Dead
	 */
	virtual void StartDead() override;
	virtual void TickDead(const float DeltaTime) override;
	virtual void EndDead() override;

	/*
	 * Owner 측에서 상태 관련 처리만 위임
	 */
	virtual void HandleDie() override;
	
	
	void HandlePipeBombBeep(AActor* PipeBombActor);
};