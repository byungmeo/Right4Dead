// Fill out your copyright notice in the Description page of Project Settings.


#include "WeponPipeBomb.h"

#include "CommonZombie.h"
#include "ExplosionDamageType.h"
#include "Camera/CameraComponent.h"
#include "Components/SphereComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Particles/ParticleSystemComponent.h"
#include "Components/SpotLightComponent.h"
#include "Right4Dead/Right4Dead.h"


// Sets default values
AWeponPipeBomb::AWeponPipeBomb()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	TrailParticle = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("Trail"));
	TrailParticle->SetupAttachment(PrimaryWeapon);

	BeepLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("BeepLight"));
	BeepLight->SetupAttachment(PrimaryWeapon);
	
	//파이프폭탄 사운드 재생하자
	const ConstructorHelpers::FObjectFinder<USoundWave>TempPipeBombBeep(TEXT("/Script/Engine.SoundWave'/Game/Assets/Sounds/WeaponNAttack/PipeBomb/pipebomb_beep.pipebomb_beep'"));
	if (TempPipeBombBeep.Succeeded())
	{
		PipeBombBeep = TempPipeBombBeep.Object;
	}
	const ConstructorHelpers::FObjectFinder<USoundWave>TempPipeBombEnd(TEXT("/Script/Engine.SoundWave'/Game/Assets/Sounds/WeaponNAttack/PipeBomb/Pipebomb_explode3.Pipebomb_explode3'"));
	if (TempPipeBombEnd.Succeeded())
	{
		PipeBombEnd = TempPipeBombEnd.Object;
	}

	//폭발 이펙트
	ConstructorHelpers::FObjectFinder<UParticleSystem>TempExplosion(TEXT("/Script/Engine.ParticleSystem'/Game/Assets/StarterContent/Particles/P_Explosion.P_Explosion'"));
	if (TempExplosion.Succeeded())
	{
		ExplosionEffect = TempExplosion.Object;
	}

}

// Called when the game starts or when spawned
void AWeponPipeBomb::BeginPlay()
{
	Super::BeginPlay();

	// 플레이어 캐스팅
	me = Cast<ASurvivor>(GetWorld()->GetFirstPlayerController()->GetPawn());
	BeepLight->SetVisibility(false);

	SetActorTickEnabled(false);
}

// Called every frame
void AWeponPipeBomb::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	// 던져지고 나서
	if (bIsThrown==true)
	{
		PlayBeepSound(DeltaTime);
	}
}

void AWeponPipeBomb::PipeBombInteraction()
{
	//투척무기
	//몽타주 특정시점(추가필요)에서 무기해제 (던지고 나서도 손에 들고있으면 안됨)
	me->CurrentWeapon->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	bIsThrown = true;
	SetActorTickEnabled(true);
	
	//몽타주 플레이
	if (me->CurrentWeapon->WeaponData.WeaponFireMontage)
	{
		me->Arms->GetAnimInstance()->Montage_Play(me->CurrentWeapon->WeaponData.WeaponFireMontage);
	}
	//SKYE: 프리셋 추가 설정
	if (bIsThrown)
	{
		me->CurrentWeapon = nullptr;
		ThrowWeapon();
	}
}

void AWeponPipeBomb::ThrowWeapon()
{
	TrailParticle->BeginTrails(TEXT("Socket"), TEXT("Socket0"), ETrailWidthMode_FromFirst, 1);
	TrailParticle->BeginTrails(TEXT("Socket1"), TEXT("Socket2"), ETrailWidthMode_FromFirst, 1);

	FVector StartLocation = me->GetActorLocation() + FVector(0, 0, 80);

	// 카메라 방향 반영
	FVector CameraForwardVector = me->FirstCameraComp->GetForwardVector();  // 카메라의 Forward Vector 사용
	FVector TargetLocation = StartLocation + CameraForwardVector * 1500; // 카메라 방향으로 목표 위치 설정
	
	//0~1사이 (포물선 궤적 높이라고 이해하자)
	float arcValue = 0.75f;
	FVector outVelocity = FVector::ZeroVector;
	if (UGameplayStatics::SuggestProjectileVelocity_CustomArc(this, outVelocity, StartLocation, TargetLocation, GetWorld()->GetGravityZ(),arcValue))
	{
		FPredictProjectilePathParams predictParams (20.0f, StartLocation, outVelocity, 15.0f);
		//->20은 트레이싱이 보여질 프로젝타일 크기 /  15는 시뮬레이션 되는 Max 시간
		if (me->bDebugPlay)
		{
			predictParams.DrawDebugTime=15.f;
			predictParams.DrawDebugType = EDrawDebugTrace::Type::ForDuration;
		}
		predictParams.OverrideGravityZ=GetWorld()->GetGravityZ();
		FPredictProjectilePathResult predictResult;
		UGameplayStatics::PredictProjectilePath(this, predictParams, predictResult);

		//이제 날려보내자
		Root->SetSimulatePhysics(true);
		Root->SetPhysicsLinearVelocity(outVelocity);
		//근데 바닥에 닿으면 멈춰야함!
		Root->OnComponentHit.AddDynamic(this, &AWeponPipeBomb::OnThrowWeaponHit);
		Root->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Root->SetCollisionResponseToAllChannels(ECR_Ignore);
		Root->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
		Root->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
		Root->SetNotifyRigidBodyCollision(true); //충돌이벤트 발생가능하게 설정
	}
}

void AWeponPipeBomb::OnThrowWeaponHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
                                 UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	//이미 바닥이면 실행하지말자
	if (bHasLanded) return;

	//바닥에 닿으면 속도를 0으로 만들자
	bHasLanded = true;
	BeepLight->SetVisibility(true);
	
	//속도 멈추기
	Root->SetPhysicsLinearVelocity(FVector::ZeroVector);
	Root->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);

	//마찰력을 증가시키자
	Root->SetLinearDamping(5.0f); //이속 감소
	Root->SetAngularDamping(5.0f); //회전 감소

	//6초동안 sphere trace로 좀비 감지하기 (범위 3000이상)
	//1초마다 loop
	GetWorld()->GetTimerManager().SetTimer(PipeBombTraceTimerHandle, this, &AWeponPipeBomb::PipeBombTraceZombies, 1.0f, true);
	//
	//6초후 폭발 타이머 설정 (폭발함수 구현하기)
	GetWorld()->GetTimerManager().SetTimer(ExplosionTimerHandle, this, &AWeponPipeBomb::ExplodeWeapon, 5.8f, false);
}

void AWeponPipeBomb::PipeBombTraceZombies()
{	
	//Sphere Trace 실행
	FVector TraceStart = GetActorLocation();
	FVector TraceEnd = TraceStart;
	float TraceRadius = 3000.0f; //감지 범위

	TArray<FHitResult> HitResults;
	FCollisionShape SphereShape = FCollisionShape::MakeSphere(TraceRadius);

	bool bHit = GetWorld()->SweepMultiByChannel(
		HitResults,
		TraceStart,
		TraceEnd,
		FQuat::Identity,
		ECC_GameTraceChannel6,
		SphereShape
	);

	//감지된 액터들 중 ACommonZombie 타입인지 확인하고 이벤트 호출
	for (const FHitResult& HitResult : HitResults)
	{
		AActor* HitActor = HitResult.GetActor();
		if (HitActor && HitActor->IsA(ACommonZombie::StaticClass()))
		{
			ACommonZombie* Zombie = Cast<ACommonZombie>(HitActor);
			if (Zombie)
			{
				Zombie->HandlePipeBombBeep(this);
			}
		}
	}
}

void AWeponPipeBomb::ExplodeWeapon()
{
	//좀비 감지 타이머 중지
	GetWorld()->GetTimerManager().ClearTimer(PipeBombTraceTimerHandle);
	GetWorldTimerManager().ClearTimer(BeepSoundTimerHandle);
	GetWorld()->GetTimerManager().ClearTimer(ExplosionTimerHandle);
	
	TArray<AActor*> IgnoreActors;
	IgnoreActors.Add(this);

	//데미지 적용
	UGameplayStatics::ApplyRadialDamage(
		this,500.f,
		GetActorLocation(),
		600.f,
		UExplosionDamageType::StaticClass(),
		IgnoreActors,
		this,
		GetWorld()->GetFirstPlayerController(),
		true,
		ECC_Visibility);

	//폭발 반경을 빨간색 구체로 표시 (2초간 유지)
	if (me->bDebugPlay)
	{
		DrawDebugSphere(
			GetWorld(),           // World
			GetActorLocation(),    // Center
			600.f,      // Radius
			32,                   // Segments
			FColor::Red,          // Color
			false,               // Persistent Lines
			5.0f);                // Duration
	}

	// 터지면 사운드를 재생하고 싶다
	UGameplayStatics::PlaySound2D(this, PipeBombEnd, 1, 1);
	// 이펙트도!
	UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ExplosionEffect, GetActorLocation());

	this->Destroy();
}

