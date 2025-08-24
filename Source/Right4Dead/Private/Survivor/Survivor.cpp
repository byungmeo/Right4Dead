// Fill out your copyright notice in the Description page of Project Settings.


#include "Survivor.h"

#include "BulletDamageType.h"
#include "CokeDelivery.h"
#include "CommonDoor.h"
#include "CommonZombie.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "EWeaponType.h"
#include "InputActionValue.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "ItemBase.h"
#include "MeleeDamageType.h"
#include "SafeDoor.h"
#include "ShoveDamageType.h"
#include "SurvivorArmAnim.h"
#include "SurvivorThirdPersonAnim.h"
#include "UIAttackZombie.h"
#include "UISurvivorCokeDelivery.h"
#include "UISurvivorCrosshair.h"
#include "UISurvivorIndicator.h"
#include "UISurvivorMain.h"
#include "UISurvivorMedKit.h"

#include "UITakeDamage.h"
#include "UIWeaponSlot.h"
#include "WeaponBase.h"
#include "WeaponCoke.h"
#include "WeaponHealKit.h"
#include "WeponPipeBomb.h"
#include "AssetTypeActions/AssetDefinition_SoundBase.h"
#include "BehaviorTree/Tasks/BTTask_PlayAnimation.h"
#include "Blueprint/UserWidget.h"
#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Logging/LogTrace.h"
#include "Right4Dead/Right4Dead.h"
#include "Tests/AutomationCommon.h"
#include "Sound/SoundCue.h"
#include "Components/AudioComponent.h"
#include "Storage/Nodes/FileEntry.h"

// Sets default values
ASurvivor::ASurvivor()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	Head=CreateDefaultSubobject<USkeletalMeshComponent>("Head");
	Head->SetupAttachment(GetMesh());

	//기본 카메라 설정
	FirstCameraComp=CreateDefaultSubobject<UCameraComponent>(TEXT("FirstCameraComp"));
	FirstCameraComp->SetupAttachment(Head);
	FirstCameraComp->SetRelativeLocationAndRotation(FVector(0,0,160),FRotator(0,90,0));
	FirstCameraComp->bUsePawnControlRotation = true;
	
	ThirdPerson=CreateDefaultSubobject<USkeletalMeshComponent>("ThirdPerson");
	ThirdPerson->SetupAttachment(RootComponent);
	ThirdPerson->SetVisibility(false);
	
	SpringArmComp=CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArmComp"));
	SpringArmComp->SetupAttachment(ThirdPerson);
	SpringArmComp->SetRelativeLocation(FVector(0,0,200));
	SpringArmComp->TargetArmLength = 300.f;
	SpringArmComp->bUsePawnControlRotation = true;
	
	ThirdPersonCameraComp=CreateDefaultSubobject<UCameraComponent>(TEXT("ThirdCameraComp"));
	ThirdPersonCameraComp->SetupAttachment(SpringArmComp);

	Arms=CreateDefaultSubobject<USkeletalMeshComponent>("Arms");
	Arms->SetupAttachment(FirstCameraComp);
	Arms->SetRelativeLocationAndRotation(FVector(-5,0,-170), FRotator(0,-90,0));
	
	bFirstPerson = true;

	//몽타주 연동
	ConstructorHelpers::FObjectFinder<UAnimMontage> TempShoveMontage(TEXT("/Script/Engine.AnimMontage'/Game/UltimateFPSAnimationsKIT/Animations/Arms/swingmontage_new.swingmontage_new'"));
	if (TempShoveMontage.Succeeded())
	{
		ShoveMontage = TempShoveMontage.Object;
	}
	ConstructorHelpers::FObjectFinder<UAnimMontage> TempCokeShoveMontage(TEXT("/Script/Engine.AnimMontage'/Game/UltimateFPSAnimationsKIT/Animations/Add/Coke/My_Anim_Attack_I_Montage.My_Anim_Attack_I_Montage'"));
	if (TempCokeShoveMontage.Succeeded())
	{
		CokeShoveMontage = TempCokeShoveMontage.Object;
	}

	//Input데이터 할당하기
	//이동관련
	ConstructorHelpers::FObjectFinder<UInputMappingContext> TempIMC(TEXT("/Script/EnhancedInput.InputMappingContext'/Game/Input/IMC_Survivor.IMC_Survivor'"));
	if (TempIMC.Succeeded())
	{
		IMC_Survivor=TempIMC.Object;
	}
	ConstructorHelpers::FObjectFinder<UInputAction> TempIAMove(TEXT("/Script/EnhancedInput.InputAction'/Game/Input/IA_SurMove.IA_SurMove'"));
	if (TempIAMove.Succeeded())
	{
		IA_SurMove=TempIAMove.Object;
	}
	ConstructorHelpers::FObjectFinder<UInputAction> TempIATurn(TEXT("/Script/EnhancedInput.InputAction'/Game/Input/IA_SurTurn.IA_SurTurn'"));
	if (TempIATurn.Succeeded())
	{
		IA_SurTurn=TempIATurn.Object;
	}
	ConstructorHelpers::FObjectFinder<UInputAction> TempIALook(TEXT("/Script/EnhancedInput.InputAction'/Game/Input/IA_SurLook.IA_SurLook'"));
	if (TempIALook.Succeeded())
	{
		IA_SurLook=TempIALook.Object;
	}
	ConstructorHelpers::FObjectFinder<UInputAction> TempIAJump(TEXT("/Script/EnhancedInput.InputAction'/Game/Input/IA_SurJump.IA_SurJump'"));
	if (TempIALook.Succeeded())
	{
		IA_SurJump=TempIAJump.Object;
	}
	ConstructorHelpers::FObjectFinder<UInputAction> TempIACrouch(TEXT("/Script/EnhancedInput.InputAction'/Game/Input/IA_SurCrouch.IA_SurCrouch'"));
	if (TempIACrouch.Succeeded())
	{
		IA_SurCrouch=TempIACrouch.Object;
	}

	//공격관련 input
	ConstructorHelpers::FObjectFinder<UInputAction> TempIAFire(TEXT("/Script/EnhancedInput.InputAction'/Game/Input/IA_SurFire.IA_SurFire'"));
	if (TempIAFire.Succeeded())
	{
		IA_SurFire=TempIAFire.Object;
	}
	ConstructorHelpers::FObjectFinder<UInputAction> TempIARelaod(TEXT("/Script/EnhancedInput.InputAction'/Game/Input/IA_SurReload.IA_SurReload'"));
	if (TempIARelaod.Succeeded())
	{
		IA_SurReload=TempIARelaod.Object;
	}
	ConstructorHelpers::FObjectFinder<UInputAction> TempIARight(TEXT("/Script/EnhancedInput.InputAction'/Game/Input/IA_SurRight.IA_SurRight'"));
	if (TempIARight.Succeeded())
	{
		IA_SurRight=TempIARight.Object;
	}

	//무기전환 input
	ConstructorHelpers::FObjectFinder<UInputAction> TempIAPrimary(TEXT("/Script/EnhancedInput.InputAction'/Game/Input/IA_Weapon1.IA_Weapon1'"));
	if (TempIAPrimary.Succeeded())
	{
		IA_PrimaryWeapon=TempIAPrimary.Object;
	}
	ConstructorHelpers::FObjectFinder<UInputAction> TempIASecondary(TEXT("/Script/EnhancedInput.InputAction'/Game/Input/IA_Weapon2.IA_Weapon2'"));
	if (TempIASecondary.Succeeded())
	{
		IA_SecondaryWeapon=TempIASecondary.Object;
	}
	ConstructorHelpers::FObjectFinder<UInputAction> TempIAMelee(TEXT("/Script/EnhancedInput.InputAction'/Game/Input/IA_Weapon3.IA_Weapon3'"));
	if (TempIAMelee.Succeeded())
	{
		IA_MeleeWeapon=TempIAMelee.Object;
	}
	ConstructorHelpers::FObjectFinder<UInputAction> TempIAHandle(TEXT("/Script/EnhancedInput.InputAction'/Game/Input/IA_Weapon4.IA_Weapon4'"));
	if (TempIAHandle.Succeeded())
	{
		IA_HandleObject=TempIAHandle.Object;
	}
	ConstructorHelpers::FObjectFinder<UInputAction> TempIACoke(TEXT("/Script/EnhancedInput.InputAction'/Game/Input/IA_Weapon6_Coke.IA_Weapon6_Coke'"));
	if (TempIACoke.Succeeded())
	{
		IA_CokeDelivery=TempIACoke.Object;
	}
	
	//무기줍기
	ConstructorHelpers::FObjectFinder<UInputAction> TempIAPickUp(TEXT("/Script/EnhancedInput.InputAction'/Game/Input/IA_SurPickUp.IA_SurPickUp'"));
	if (TempIAPickUp.Succeeded())
	{
		IA_PickUp=TempIAPickUp.Object;
	}

	//overlap되면 Material Instance (Overlay 설정)
	static ConstructorHelpers::FObjectFinder<UMaterialInstance> TempWeaponOverlay(TEXT("/Script/Engine.MaterialInstanceConstant'/Game/Blueprints/Survivor/Materials/M_Outline_Inst.M_Outline_Inst'"));
	if (TempWeaponOverlay.Succeeded())
	{
		OverlayMaterial=TempWeaponOverlay.Object;
	}

	// 실린더 메시 설정
	ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMeshAsset(TEXT("/Script/Engine.StaticMesh'/Engine/BasicShapes/Cylinder.Cylinder'"));
	if (CylinderMeshAsset.Succeeded())
	{
		CylinderMesh = CylinderMeshAsset.Object;
	}

	//BoneMap 초기화 (뼈의 이름과 우선순위 매핑))
	//TODO: None이면 예외 처리 해야함

	// 머리
    BonePriorityMap.Add(TEXT("head"), 0);
	BonePriorityMap.Add(TEXT("neck_01"), 0);

	// 가슴
	BonePriorityMap.Add(TEXT("spine_02"), 1);

	// 배
	BonePriorityMap.Add(TEXT("pelvis"), 2);
	BonePriorityMap.Add(TEXT("spine_01"), 2);

	// 팔
	BonePriorityMap.Add(TEXT("upperarm_l"), 3);
	BonePriorityMap.Add(TEXT("lowerarm_l"), 3);
	BonePriorityMap.Add(TEXT("hand_l"), 3);
	BonePriorityMap.Add(TEXT("upperarm_r"), 3);
	BonePriorityMap.Add(TEXT("lowerarm_r"), 3);
	BonePriorityMap.Add(TEXT("hand_r"), 3);

	// 다리
	BonePriorityMap.Add(TEXT("thigh_l"), 3);
	BonePriorityMap.Add(TEXT("calf_l"), 3);
	BonePriorityMap.Add(TEXT("foot_l"), 3);
	BonePriorityMap.Add(TEXT("ball_l"), 3);
	BonePriorityMap.Add(TEXT("thigh_r"), 3);
	BonePriorityMap.Add(TEXT("calf_r"), 3);
	BonePriorityMap.Add(TEXT("foot_r"), 3);
	BonePriorityMap.Add(TEXT("ball_r"), 3);

	//사운드 재생
	//도끼
	const ConstructorHelpers::FObjectFinder<USoundWave>TempSwingMiss(TEXT("/Script/Engine.SoundWave'/Game/Assets/Sounds/WeaponNAttack/swing_miss1.swing_miss1'"));
	if (TempSwingMiss.Succeeded())
	{
		SwingMiss = TempSwingMiss.Object;
	}
	const ConstructorHelpers::FObjectFinder<USoundWave>TempSwingHitZombie(TEXT("/Script/Engine.SoundWave'/Game/Assets/Sounds/WeaponNAttack/Melee/Axe/axe_03.axe_03'"));
	if (TempSwingHitZombie.Succeeded())
	{
		SwingHitZombie = TempSwingHitZombie.Object;
	}
	const ConstructorHelpers::FObjectFinder<USoundWave>TempSwingHitWorld(TEXT("/Script/Engine.SoundWave'/Game/Assets/Sounds/WeaponNAttack/Melee/melee_swing_hit_world.melee_swing_hit_world'"));
	if (TempSwingHitWorld.Succeeded())
	{
		SwingHitZombie = TempSwingHitWorld.Object;
	}
	//무기 있을때 우클릭
	const ConstructorHelpers::FObjectFinder<USoundWave>TempWeaponSwing(TEXT("/Script/Engine.SoundWave'/Game/Assets/Sounds/WeaponNAttack/swish_weaponswing_swipe5.swish_weaponswing_swipe5'"));
	if (TempWeaponSwing.Succeeded())
	{
		WeaponSwing = TempWeaponSwing.Object;
	}
	//맞았을때 사운드
	const ConstructorHelpers::FObjectFinder<USoundCue>TempTakeDamage(TEXT("/Script/Engine.SoundCue'/Game/Assets/Sounds/Survivor/PlayerHurt.PlayerHurt'"));
	if (TempTakeDamage.Succeeded())
	{
		TakeDamageSound = TempTakeDamage.Object;
	}

	//콜라배달 사운드
	ColaDeliveryAudio = CreateDefaultSubobject<UAudioComponent>(TEXT("ColaDeliveryAudio"));
	ColaDeliveryAudio->bAutoActivate = false; // 자동 실행 방지코드라고 함
	ColaDeliveryAudio->SetupAttachment(RootComponent);
	
	const ConstructorHelpers::FObjectFinder<USoundWave>TempCoke(TEXT("/Script/Engine.SoundWave'/Game/Assets/Sounds/Item/Coke/attach_cola_bottles_01.attach_cola_bottles_01'"));
	if (TempCoke.Succeeded())
	{
		CokeDeliverySound = TempCoke.Object;
	}
	
	if (CokeDeliverySound)
	{
		ColaDeliveryAudio->SetSound(CokeDeliverySound);
	}
	
}

//무기, 아이템과 박스가 오버랩 됐을때
void ASurvivor::OnWeaponOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
	const FHitResult& SweepResult)
{
	AWeaponBase* OverlapWeapon = Cast<AWeaponBase>(OtherActor);
	if (OverlapWeapon)
	{
		OverlapWeapon->SetOverlayMaterial(OverlayMaterial);
	}
	AItemBase* OverlapItem = Cast<AItemBase>(OtherActor);
	if (OverlapItem)
	{
		OverlapItem->SetOverlayMaterial(OverlayMaterial);
		//preset - weapon overlap과 overlap으로 체크해야함
	}
}

//무기, 아이템과 박스가 오버랩 해제
void ASurvivor::OnWeaponEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	AWeaponBase* OverlapWeapon = Cast<AWeaponBase>(OtherActor);
	if (OverlapWeapon)
	{
		OverlapWeapon->ClearOverlayMaterial();
	}
	AItemBase* OverlapItem = Cast<AItemBase>(OtherActor);
	if (OverlapItem)
	{
		OverlapItem->ClearOverlayMaterial();
	}
}

// Called when the game starts or when spawned
void ASurvivor::BeginPlay()
{
	Super::BeginPlay();
	
	//Enhanced Input
	auto* pc = Cast<APlayerController>(Controller);
	if (pc)
	{
		auto subSystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(pc->GetLocalPlayer());
		if (subSystem)
		{
			subSystem->AddMappingContext(IMC_Survivor,0);
		}
	}

	//UI붙이기
	SurvivorMainUI=Cast<UUISurvivorMain>(CreateWidget(GetWorld(),MainUIFactory));
	if (SurvivorMainUI)
	{
		SurvivorMainUI->AddToViewport();
		CurrentHP=MaxHP;
		CrosshairUI = Cast<UUISurvivorCrosshair>(SurvivorMainUI->Crosshair);
	}
	if (TakeDamageUIClass) // UPROPERTY로 설정된 UClass<UUserWidget> 변수
	{
		TakeDamageUI = CreateWidget<UUITakeDamage>(GetWorld(), TakeDamageUIClass);
		if (TakeDamageUI)
		{
			TakeDamageUI->AddToViewport();
		}
	}
	if (AttackZombieUIClass)
	{
		AttackZombieUI = CreateWidget<UUIAttackZombie>(GetWorld(), AttackZombieUIClass);
		if (AttackZombieUI)
		{
			AttackZombieUI->AddToViewport();
		}
	}
	if (MedKitUIClass)
	{
		MedKitUI = CreateWidget<UUISurvivorMedKit>(GetWorld(), MedKitUIClass);
		if (MedKitUI)
		{
			MedKitUI->SetVisibility(ESlateVisibility::Hidden);
			MedKitUI->AddToViewport();
		}
	}
	if (CokeDeliveryUIClass)
	{
		CokeDeliveryUI = CreateWidget<UUISurvivorCokeDelivery>(GetWorld(), CokeDeliveryUIClass);
		if (CokeDeliveryUI)
		{
			CokeDeliveryUI->SetVisibility(ESlateVisibility::Hidden);
			CokeDeliveryUI->AddToViewport();
		}
	}
	
		
	//카메라 설정
	FirstCameraComp->SetActive(true);
	ThirdPersonCameraComp->SetActive(false);
	SpringArmComp->SetActive(false);


	// Inventory를 초기화 하고 싶다
	// 슬롯 종류의 총 갯수만큼 Inventory를 늘리고 싶다.
	constexpr int32 SlotTypeNumber = static_cast<int32>(EWeaponType::MAX);
	for (int i = 0; i < SlotTypeNumber; i++)
	{
		Inventory.Add(static_cast<EWeaponType>(i), nullptr);
	}

	// 도끼 쿨타임 초기화 (시작해도 바로 공격할 수 있게)
	LastSecondaryAttackTime = -1;
}

// Called every frame
void ASurvivor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	//direction을 캐릭터가 바라보는 방향으로 변환
	Direction=FTransform(GetControlRotation()).TransformVector(Direction);

	//이동 : 캐릭터 무브먼트 이용
	AddMovementInput(Direction);
	Direction=FVector::Zero();

	//트레이스 후 무기면 -> 줍기
	TraceForWeapon();

	// 조준선 UI 업데이트
	float Value = UKismetMathLibrary::VSize(GetVelocity());
	if (CrosshairUI)
	{
		CrosshairUI->CrosshairSpread = UKismetMathLibrary::MapRangeClamped(Value,0,450,5,70);
	}

	// 좌클릭을 꾹 누르고 있으면 회복 (5초동안 꾹누르기)
	if (bIsHoldingLeft)
	{
		HoldTime += GetWorld()->GetDeltaSeconds();
		if (!bIsHoldThresholdReached && HoldTime >= HoldThresholdTime)
		{
			bIsHoldThresholdReached = true;
		}

		// HoldTime이 MaxHoldTime을 초과하면
		// 키트 : 회복 및 카메라 전환
		// 콜라 : 배달
		if (HoldTime >= MaxHoldTime)
		{
			HandleHoldComplete();
		}
	}
}

// Called to bind functionality to input
void ASurvivor::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	auto pi=CastChecked<UEnhancedInputComponent>(PlayerInputComponent);
	if (pi)
	{
		pi->BindAction(IA_SurMove, ETriggerEvent::Triggered, this, &ASurvivor::SurvivorMove);
		pi->BindAction(IA_SurLook, ETriggerEvent::Triggered, this, &ASurvivor::SurvivorLook);
		pi->BindAction(IA_SurTurn, ETriggerEvent::Triggered, this, &ASurvivor::SurvivorTurn);
		pi->BindAction(IA_SurJump, ETriggerEvent::Started, this, &ASurvivor::SurvivorJump);
		pi->BindAction(IA_SurCrouch, ETriggerEvent::Started, this, &ASurvivor::SurvivorCrouch);
		pi->BindAction(IA_SurFire, ETriggerEvent::Started, this, &ASurvivor::LeftClickAttack);

		pi->BindAction(IA_SurFire, ETriggerEvent::Started, this, &ASurvivor::HandleSingleClickAttack);
		pi->BindAction(IA_SurFireHold, ETriggerEvent::Started, this, &ASurvivor::HandleHoldAttack);
		pi->BindAction(IA_SurFireHold, ETriggerEvent::Canceled, this, &ASurvivor::HandleReleaseAttack);
		
		pi->BindAction(IA_SurRight, ETriggerEvent::Started, this, &ASurvivor::RightClickAttack);
		pi->BindAction(IA_SurReload, ETriggerEvent::Started, this, &ASurvivor::WeaponReload);
		pi->BindAction(IA_PrimaryWeapon, ETriggerEvent::Started, this, &ASurvivor::EquipPrimaryWeapon);
		pi->BindAction(IA_SecondaryWeapon, ETriggerEvent::Started, this, &ASurvivor::EquipSecondaryWeapon);
		pi->BindAction(IA_MeleeWeapon, ETriggerEvent::Started, this, &ASurvivor::EquipMeleeWeapon);
		pi->BindAction(IA_HandleObject, ETriggerEvent::Started, this, &ASurvivor::EquipHandleObject);
		pi->BindAction(IA_CokeDelivery, ETriggerEvent::Started, this, &ASurvivor::EquipCokeBox);
		pi->BindAction(IA_PickUp,ETriggerEvent::Started,this,&ASurvivor::PickUpWeapon_Input);
	}
}

//카메라 전환 함수 
void ASurvivor::SwitchCamera(const bool& bThirdPerson)
{
	// bFirstPerson = !bFirstPerson;
	if (false == bThirdPerson)
	{
		// 1인칭이면
		FirstCameraComp->SetActive(true);
		ThirdPersonCameraComp->SetActive(false);
		SpringArmComp->SetActive(false);
		Arms->SetVisibility(true);
		CurrentWeapon->SetHidden(false);
		ThirdPerson->SetVisibility(false);
	}
	else
	{
		// 3인칭이면
		FirstCameraComp->SetActive(false);
		ThirdPersonCameraComp->SetActive(true);
		SpringArmComp->SetActive(true);
		Arms->SetVisibility(false);
		CurrentWeapon->SetHidden(true);
		ThirdPerson->SetVisibility(true);
	}
}



void ASurvivor::SurvivorMove(const struct FInputActionValue& InputValue)
{
	FVector2d dir=InputValue.Get<FVector2d>();
	Direction.X = dir.Y;
	Direction.Y = dir.X;
}

void ASurvivor::SurvivorCrouch(const struct FInputActionValue& InputValue)
{
	Crouch();
}

void ASurvivor::SurvivorTurn(const struct FInputActionValue& InputValue)
{
	float value = InputValue.Get<float>();
	AddControllerYawInput(value);
}

void ASurvivor::SurvivorLook(const struct FInputActionValue& InputValue)
{
	float value = InputValue.Get<float>();
	AddControllerPitchInput(value);
}

void ASurvivor::SurvivorJump(const struct FInputActionValue& InputValue)
{
	Jump();
}

float ASurvivor::TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent,
							class AController* EventInstigator, AActor* DamageCauser)
{
	Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	OnDamaged(DamageAmount);
	if (false == bGod)
	{
		DisplayIndicator(DamageCauser);
	}
	return DamageAmount;
}

void ASurvivor::OnDamaged(float Damage)
{
	if (bGod)
	{
		return;
	}
	
	bIsDamaged=true;
	
	//카메라 쉐이크
	auto pc = GetWorld()->GetFirstPlayerController();
	if (pc)
	{
		pc->PlayerCameraManager->StartCameraShake(DamagedCameraShake);
	}
	// 위젯 애니메이션 재생
	if (TakeDamageUI)
	{
		TakeDamageUI->PlayAnimationByName();
	}
	//체력깎기
	CurrentHP -= Damage;
	//맞는 소리 재생
	UGameplayStatics::PlaySound2D(this, TakeDamageSound, 1, 1);
	
	//0되면 ondie호출하기
	if (CurrentHP <= 0)
	{
		CurrentHP = 0;
		OnDie();
	}
}

void ASurvivor::OnDie()
{
	bIsDead = true;
	
}

void ASurvivor::LeftClickAttack(const struct FInputActionValue& InputValue)
{
	UAnimInstance* AnimInst = Arms->GetAnimInstance();
	USurvivorArmAnim* WeaponInst = Cast<USurvivorArmAnim>(AnimInst);
	bool bIsEquipped = WeaponInst->bIsEquippedWeapon;

	//SKYE: 슬롯을 처음에 구분후 슬롯별 무기 공격 추가 필요함
	if (CurrentWeapon && bIsEquipped) //무기가 있을때만 가능
	{
		switch (CurrentWeaponSlot->WeaponName)
		{
		case EWeaponType::Primary:
			PrimaryWeaponAttack();
			break;
		case EWeaponType::Secondary:
			SecondaryWeaponAttack();
			break;
		case EWeaponType::Melee:
			MeleeWeaponAttack();
			break;
		/*case EWeaponType::HandleObject:
			HandleHoldAttack();
			break;*/
		/*case EWeaponType::CokeDelivery:
			HandleHoldAttack();
			break;*/
		case EWeaponType::None:
			NoneAttack();
			break;
		}
	}
	else
		NoneAttack();
}

void ASurvivor::PrimaryWeaponAttack()
{
	// 총알이 있어야....
	if (CurrentWeapon && CurrentWeapon->WeaponData.CurrentAmmo > 0)
	{
		//총무기 라인트레이스
		//TODO: 나중에 따발총 추가되면 변수추가해서 바꿔야함
		FHitResult Hit;
		FCollisionQueryParams Params;
		Params.AddIgnoredActor(this);
		
		APlayerCameraManager* FirstCam = UGameplayStatics::GetPlayerCameraManager(GetWorld(),0);
		if (!FirstCam)
		{
			return;
		}
		FVector Start = FirstCam->GetCameraLocation();
		FVector End = Start + (FirstCam->GetActorForwardVector() * 30000);
		    
		const float DebugLineLifetime = 2.0f;
		bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_GameTraceChannel6, Params);
			
		// 디버그 라인 그리기
		if (bDebugPlay)
		{
			if (bHit)
			{
				// 히트가 발생한 경우 빨간색으로 표시
				DrawDebugLine(GetWorld(), Start, Hit.Location, FColor::Red, false, DebugLineLifetime, 0, 0.5f);
				FString BoneNameStr = FString::Printf(TEXT("Hit Bone: %s"), *Hit.BoneName.ToString());
				UE_LOG(LogTemp, Warning, TEXT("%s %s"), *Hit.GetActor()->GetName(), *BoneNameStr);
			}
			else
			{
				// 히트가 없는 경우 초록색으로 표시
				DrawDebugLine(GetWorld(),Start,End,FColor::Green,false,DebugLineLifetime, 0,0.5f);
			}
		}
		    
		if (bHit && Hit.GetActor())
		{
			if (false == Hit.BoneName.IsNone())
			{
				const FVector HitFromDirection = (GetActorForwardVector() + FVector(0, 0, 0.5f)).GetSafeNormal();
				UGameplayStatics::ApplyPointDamage(Hit.GetActor(), 10, HitFromDirection, Hit, nullptr, nullptr, UBulletDamageType::StaticClass());
			}
		}

		//총알 개수 줄이자
		DecreaseAmmoCount();

		//몽타주 플레이
		if (CurrentWeapon->WeaponData.WeaponFireMontage)
		{
			Arms->GetAnimInstance()->Montage_Play(CurrentWeapon->WeaponData.WeaponFireMontage);
		}
		else
		{
			return;
		}

		//카메라 쉐이크
		auto pc = GetWorld()->GetFirstPlayerController();
		if (pc)
		{
			pc->PlayerCameraManager->StartCameraShake(GunCameraShake);
		}
	}
	else
	{
		return;
	}
}


void ASurvivor::SecondaryWeaponAttack()
{
	if (!GetWorld()) return;

	// 시간 계산
	const float CurrentTime = GetWorld()->GetTimeSeconds();
	if (CurrentTime - LastSecondaryAttackTime < SecondaryAttackCooldown)
	{
		return;
	}

	// 공격 로직 실행
	LastSecondaryAttackTime = CurrentTime;

	//근접무기 휘두르기
	Sweep();
	if (UAnimInstance* AnimInstance = Arms->GetAnimInstance())
	{
		PRINT_CALLINFO();
		AnimInstance->Montage_Play(SecondaryWeaponSlot.WeaponFireMontage);
	}
}

void ASurvivor::MeleeWeaponAttack()
{
	// pipebomb 내부 함수 호출
	auto* pipeBomb = Cast<AWeponPipeBomb>(CurrentWeapon);
	pipeBomb->PipeBombInteraction();
}


void ASurvivor::HandleSingleClickAttack()
{
	// 만약, 무기를 들고 있지 않다면 아무것도 하지 않는다.
	if (nullptr == CurrentWeapon || false == CurrentWeaponSlot.IsSet())
	{
		return;
	}
	
	UAnimInstance* AnimInst = Arms->GetAnimInstance();
	USurvivorArmAnim* WeaponInst = Cast<USurvivorArmAnim>(AnimInst);
	bool bIsEquipped = WeaponInst->bIsEquippedWeapon;
	
	if (bIsHoldingLeft)
	{
		return;
	}

	//짜잔(변수정리법)
	//응급키트거나 콜라일떄만
	const bool bIsHandleWeapon = CurrentWeaponSlot->WeaponName == EWeaponType::HandleObject;
	const bool bIsCoke = CurrentWeaponSlot->WeaponName == EWeaponType::CokeDelivery;
	
	if (!bIsHoldThresholdReached && bIsEquipped && (bIsHandleWeapon || bIsCoke))
	{
		// 응급키트면
		auto* HealKit = Cast<AWeaponHealKit>(CurrentWeapon);
		if (HealKit)
		{
			// 한번만 눌렀을때 실행되어야하는 내용
			if (UAnimInstance* AnimInstance = Arms->GetAnimInstance())
			{
				AnimInstance->Montage_Play(ShoveMontage);
			}
			SwitchCamera(false);

			{
				if (CurrentHP!=MaxHP)
				{
					if (MedKitUI)
					{
						MedKitUI->SetVisibility(ESlateVisibility::Visible);
						MedKitUI->AddToViewport();
					}

					CurrentWeapon->PrimaryWeapon->SetVisibility(false);
					// 응급 키트를 든 상태에서 꾹 누르면 카메라를 3인칭으로 전환
					SwitchCamera(true);
				}
			}
		}
		
		// 콜라면
		auto* Coke = Cast<AWeaponCoke>(CurrentWeapon);
		if (Coke)
		{
			//한번만 눌렀을때
			if (!bCanDeliveryCola)
			{
				if (UAnimInstance* AnimInstance = Arms->GetAnimInstance())
				{
					AnimInstance->Montage_Play(CokeShoveMontage);
					// 짤그락 사운드
					UGameplayStatics::PlaySound2D(this, WeaponSwing, 1, 1);
				}
				SwitchCamera(false);
			}
		}
	}
}

void ASurvivor::HandleHoldAttack()
{
	// 만약, 무기를 들고 있지 않다면 아무것도 하지 않는다.
	if (nullptr == CurrentWeapon || false == CurrentWeaponSlot.IsSet())
	{
		return;
	}

	
	if (CurrentWeapon->SlotType == EWeaponType::Primary)
	{
		GetWorld()->GetTimerManager().SetTimer(FiredTimer, this, &ASurvivor::PrimaryWeaponAttack, 0.1f, true);
	}
	
	// 좌클릭을 꾹 누르고 있으면 시작
	bIsHoldingLeft = true;
	HoldTime = 0.0f;
	bIsHoldThresholdReached = false;
	
	// 응급키트면
	auto* HealKit = Cast<AWeaponHealKit>(CurrentWeapon);
	if (HealKit)
	{
		if (CurrentHP!=MaxHP)
		{
			if (MedKitUI)
			{
				MedKitUI->SetVisibility(ESlateVisibility::Visible);
				MedKitUI->AddToViewport();
			}
			
			CurrentWeapon->PrimaryWeapon->SetVisibility(false);
			// 응급 키트를 든 상태에서 꾹 누르면 카메라를 3인칭으로 전환
			SwitchCamera(true);
		}
	}

	// 콜라면
	auto* Coke = Cast<AWeaponCoke>(CurrentWeapon);
	if (Coke)
	{
		if (bCanDeliveryCola)
		{
			auto* CokeDelivery = Cast<ACokeDelivery>(UGameplayStatics::GetActorOfClass(GetWorld(), ACokeDelivery::StaticClass()));
			CokeDelivery->bIsCanOpen = true;
			CokeDelivery->InteractionDelivery();
			if (CokeDeliveryUI)
			{
				CokeDeliveryUI->SetVisibility(ESlateVisibility::Visible);
				CokeDeliveryUI->AddToViewport();
			}

			//콜라배달음
			ColaDeliveryAudio->Play();
			CurrentWeapon->PrimaryWeapon->SetVisibility(false);
			// 콜라병을 든 상태에서 꾹 누르면 카메라를 3인칭으로 전환
			SwitchCamera(true);
		}
	}
}

void ASurvivor::HandleReleaseAttack()
{
	// 만약, 무기를 들고 있지 않다면 아무것도 하지 않는다.
	if (nullptr == CurrentWeapon || false == CurrentWeaponSlot.IsSet())
	{
		return;
	}
	
	// 5초 되기전 놓으면~
	if (bIsHoldingLeft || CurrentWeapon->WeaponData.CurrentAmmo <= 0)
	{
		if (CurrentWeapon->SlotType == EWeaponType::Primary)
		{
			GetWorld()->GetTimerManager().ClearTimer(FiredTimer);
		}
		bIsHoldingLeft = false;
		
		if (HoldTime<HoldThresholdTime)
		{
			HandleSingleClickAttack();
			if (MedKitUI)
			{
				MedKitUI->SetVisibility(ESlateVisibility::Hidden);
				MedKitUI->RemoveFromParent();
			}
			if (CokeDeliveryUI)
			{
				CokeDeliveryUI->SetVisibility(ESlateVisibility::Hidden);
				CokeDeliveryUI->RemoveFromParent();
			}
		}
		else if (HoldTime<MaxHoldTime)
		{
			// 응급키트면
			auto* HealKit = Cast<AWeaponHealKit>(CurrentWeapon);
			if (HealKit)
			{
				if (MedKitUI)
				{
					MedKitUI->SetVisibility(ESlateVisibility::Hidden);
					MedKitUI->RemoveFromParent();
				}			
			}
			
			// 콜라면
			auto* Coke = Cast<AWeaponCoke>(CurrentWeapon);			
			if (Coke)
			{
				auto* CokeDelivery = Cast<ACokeDelivery>(UGameplayStatics::GetActorOfClass(GetWorld(), ACokeDelivery::StaticClass()));
				CokeDelivery->bIsCanOpen = false;
				if (CokeDeliveryUI)
				{
					CokeDeliveryUI->SetVisibility(ESlateVisibility::Hidden);
					CokeDeliveryUI->RemoveFromParent();
				}
				ColaDeliveryAudio->Stop();
			}
		}
		
		//프로그레스바 초기화
		HoldTime=0.0f;
		bIsHoldThresholdReached = false;
		CurrentWeapon->PrimaryWeapon->SetVisibility(true);
		// 카메라 전환
		SwitchCamera(false);
	}
}

void ASurvivor::HandleHoldComplete()
{
	auto* Heal = Cast<AWeaponHealKit>(CurrentWeapon);
	auto* Coke = Cast<AWeaponCoke>(CurrentWeapon);
	if (nullptr == Heal && nullptr == Coke)
	{
		return;
	}
	if (Heal)
	{
		// 잃은 체력의 80% 회복
		CurrentHP += 0.8f * (MaxHP - CurrentHP);

		// 응급도구 삭제
		if (HandleObjectSlot.WeaponFactory)
		{
			UnequipWeapon();
			HandleObjectSlot = FWeaponData();
		}

		if (MedKitUI)
		{
			MedKitUI->SetVisibility(ESlateVisibility::Hidden);
			MedKitUI->RemoveFromParent();
		}
	}

	
	if (Coke)
	{
		if (CokeDeliveryUI)
		{
			CokeDeliveryUI->SetVisibility(ESlateVisibility::Hidden);
			CokeDeliveryUI->RemoveFromParent();
		}

		FinisedDeliveryCoke();
	}

	// 5초가 지나면 카메라를 원래대로 전환하고 싶다
	SwitchCamera(false);

	// 콜라병 삭제
	if (CurrentWeapon->WeaponData.WeaponName == EWeaponType::CokeDelivery)
	{
		ReturnToPreviousWeapon();
		return;
	}
	
	HoldTime = 0.0f;
	bIsHoldingLeft = false;
	bIsHoldThresholdReached = false;
	
	CurrentWeaponSlot.Reset();
	CurrentWeapon = nullptr;
}


void ASurvivor::NoneAttack()
{
	if (UAnimInstance* AnimInstance = Arms->GetAnimInstance())
	{
		AnimInstance->Montage_Play(ShoveMontage);
		// 무기없을때 사운드 재생
		UGameplayStatics::PlaySound2D(this, SwingMiss, 1, 1);
	}
}

void ASurvivor::Sweep()
{
	// 스윕을 하면 사운드를 재생하고 싶다 (맞든 안맞든 재생할거임)
	UGameplayStatics::PlaySound2D(this, SwingMiss, 1, 1);
	
	// 충돌을 위한 가상의 박스 생성
    auto BoxShape = FCollisionShape::MakeBox(FVector(100, 100, 5));
   
    // 충돌결과 저장을 위한 배열 선언
    // SweepMultiByChannel이 수행되면 여기에 HitResult 구조체(충돌과 관련된 정보들이 들어있음)들이 쌓인다
    TArray<struct FHitResult> SweepHitResults;
  
    // 시작과 끝점 (박스의 중심), 현재의 80은 캐릭터의 머리위치 정도인듯
    // 시작 지점과 끝 지점은 같도록 하면 된다 (Z축 좌표를 모니터 정 중앙 위치를 기준으로)

	FVector Start,End;
	FRotator CameraRotation;
	FVector BoxLocation;
	
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (PC)
	{
		int32 ViewportX, ViewportY;
		PC->GetViewportSize(ViewportX, ViewportY);

		float CenterX = ViewportX / 2.f;
		float CenterY = ViewportY / 2.f;

		FVector WorldLocation;
		FVector WorldDirection;
		bool bDeprojectSuccess = PC->DeprojectScreenPositionToWorld(CenterX, CenterY, WorldLocation, WorldDirection);

		if (bDeprojectSuccess)
		{
			// 뷰포트 중심을 기준으로 시작점과 끝점 설정
			Start = WorldLocation;
			End = Start + (WorldDirection * 500.f); // 적당한 거리로 설정

			// 박스의 위치를 뷰포트 중심으로 설정
			BoxLocation = Start;
			CameraRotation = PC->PlayerCameraManager->GetCameraRotation();
		}
	}
	
    // 나 자신은 제외하고 싶다 (필요하면 무기 액터 같은 것들도 제외하면 된다)
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(this);

    // SweepMultiByChannel 수행
    const bool bHit = GetWorld()->SweepMultiByChannel(SweepHitResults, Start, End, CameraRotation.Quaternion(), ECC_GameTraceChannel10, BoxShape, Params);
	// ->HitResults 배열에 충돌 결과들이 저장된다

	if (bDebugPlay)
	{
		DrawDebugBox(GetWorld(), BoxLocation, FVector(100, 100, 5), CameraRotation.Quaternion(), FColor::Red, true, 3.0f);
	}
	
	//5. 충돌결과 처리
	// 만약 가상의 박스 안에 뭔가가 있었다면?
	if (bHit)
	{
	    //좀비 사운드 재생하자
	    UGameplayStatics::PlaySound2D(this, SwingHitZombie, 1, 1);
	    
	    //공격을 맞췄다는 변수 true
	    bIsAttacked = true;
	    AttackZombieUI->PlayAnimationByName(this);
	    
		// 한 액터(좀비)의 여러 부위(왼쪽팔, 머리, 오른쪽팔)가 박스 영역 안에 동시에 들어왔을때 분류 (한 번만 타격!)
	    TMap<AActor*, TArray<FName>> ActorHitBoneMap;

	    // SweepHitResults에서 각각의 HitResult를 꺼내서 확인하고 액터별 피격 부위를 ActorHitBoneMap에 저장한다.
	    for (const FHitResult& HitResult : SweepHitResults)
	    {
			// 만약 BoneName이 None이라면 SkeletalMesh가 아니라는 뜻이다. 스킵하자.
	        FName BoneName = HitResult.BoneName;
	        if (HitResult.BoneName.IsNone())
			{
				continue;
	        }

			// 좀비가 아니라면 스킵하자.
	        AActor* Actor = Cast<AZombieBase>(HitResult.GetActor());
	        if (nullptr == Actor)
	        {
	            continue;
	        }
	        
			// Actor의 BoneName 부위가 피격당했다
	        // 해당 Actor가 아직 없다면 새로운 배열(TArray))을 만들고 피격부위(BoneName)를 추가한 후 HitMAp에 저장
	        if (false == ActorHitBoneMap.Contains(Actor))
	        {
	           TArray<FName> BoneArray;
	           BoneArray.Add(BoneName);
	           ActorHitBoneMap.Add(Actor, BoneArray);
	        }
	        // 이미 존재한다면 기존의 TArray를 가져와서 BoneName을 추가한다.
	        else
	        {
	           /*auto Array = HitMap[Actor];
	           Array.Add(BoneName);
	           -> Array는 복사본이므로 원본에 영향을 주지 않는다고 함*/
	           ActorHitBoneMap[Actor].Add(BoneName); //기존 배열에 직접 추가
	        }
		}

		// ActorHitBoneMap의 Key값은 각각의 좀비를 의미하니 GetKeys를 통해 어떤 좀비들이 피격을 당했는지 알아낸다.
	    TArray<AActor*> HitZombies;
	    ActorHitBoneMap.GetKeys(HitZombies);
	  
	    // 피격을 당한 좀비들을 하나하나 알아본다.
	    for (AActor* HitZombie : HitZombies)
	    {
			int HighPriority = INT_MAX;
	        FName HighPriorityBoneName = NAME_None;
	        // 피격 당한 부위들을 하나하나 살펴본다.
			for (const FName& BoneName : ActorHitBoneMap[HitZombie])
			{
				// TMap<FName, int> BonePriorityMap : 미리 정의된 BoneName별 우선순위 Map
		        const int Priority = BonePriorityMap[BoneName];
		        if (Priority < HighPriority)
				{
					HighPriority = Priority;
		            HighPriorityBoneName = BoneName;
		        }
		    }
	        
       		// 어떤 부위들을 피격 당했는지 알았으니 우선순위가 가장 높은 Bone에 맞았다고 하고 데미지 주기
	        FHitResult FinalHitResult;
	        FinalHitResult.BoneName = HighPriorityBoneName;
	    	UGameplayStatics::ApplyPointDamage(HitZombie, ..., FinalHitResult, ...);
	    	UGameplayStatics::ApplyPointDamage(HitZombie, 9999,
	    		GetActorRightVector() * -1.0f, FinalHitResult, GetController(),
	    		this, UMeleeDamageType::StaticClass());
	    }
	}
	else
	{
		// 헛스윙 사운드 재생
	}
}

//장전
void ASurvivor::WeaponReload(const struct FInputActionValue& InputValue)
{
	bIsReloading = true;
	//총일때만 가능합니다
	if (CurrentWeapon && CurrentWeapon->SlotType == EWeaponType::Primary)
	{
		// 총알이 아직 부족하고, 남은 탄약이 존재하는 경우에만 장전 가능
		if (CurrentWeapon->WeaponData.CurrentAmmo < CurrentWeapon->WeaponData.MaxAmmo &&
			CurrentWeapon->WeaponData.MaxAmmoAmount > 0)
		{
			// 현재 총기에 남아있는 탄약과 필요한 탄약 계산
			int32 NeededAmmo = CurrentWeapon->WeaponData.MaxAmmo - CurrentWeapon->WeaponData.CurrentAmmo;
			// gpt의 정리...
			int32 AmmoToLoad = FMath::Min(NeededAmmo, CurrentWeapon->WeaponData.MaxAmmoAmount);

			// 장전된 탄 수 반영
			CurrentWeapon->WeaponData.CurrentAmmo += AmmoToLoad;
			CurrentWeapon->WeaponData.MaxAmmoAmount -= AmmoToLoad;

			// 몽타주 플레이
			if (UAnimInstance* AnimInstance = Arms->GetAnimInstance())
			{
				AnimInstance->Montage_Play(CurrentWeapon->WeaponData.WeaponReloadMontage);
			}
		}
	}
}

void ASurvivor::DecreaseAmmoCount()
{
	if (CurrentWeapon->WeaponData.CurrentAmmo > 0)
	{
		CurrentWeapon->WeaponData.CurrentAmmo--;
	}
}

//우클릭시 밀쳐내기
void ASurvivor::RightClickAttack(const struct FInputActionValue& InputValue)
{
	if (!CurrentWeapon)
	{
		UE_LOG(LogTemp, Warning, TEXT("CurrentWeapon nullptr"));
	}
	
	//사운드를 재생하자
	if (CurrentWeapon && CurrentWeapon->SlotType != EWeaponType::None)
	{
		// 무기 장착하면 짤그락
		UGameplayStatics::PlaySound2D(this, WeaponSwing, 1, 1);
	}
	else
	{
		return;
	}
		
	//TODO: 무기없을때 우클릭 모션 해결하기
	if (UAnimInstance* AnimInstance = Arms->GetAnimInstance())
	{
		auto* Coke = Cast<AWeaponCoke>(CurrentWeapon);
		if (Coke)
		{
			AnimInstance->Montage_Play(CokeShoveMontage);
		}
		else
			AnimInstance->Montage_Play(ShoveMontage);
    }
}

void ASurvivor::OnShoveOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
    const FHitResult& SweepResult)
{
    ACommonZombie* CommonZombie = Cast<ACommonZombie>(OtherActor);
	//조건1: 좀비가 맞았는지?
    if (CommonZombie)
    {
        //조건2: 좀비가 플레이어 전방 기준 좌우 45도 안에 있는가?
        FVector ZombieLocation = (CommonZombie->GetActorLocation() - GetActorLocation()).GetSafeNormal();
        FVector SurvivorForwardVector = GetActorForwardVector();

    	//내적으로 각도 구하기
        float Angle = FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(SurvivorForwardVector, ZombieLocation)));
        if (Angle <= 45.0f)
        {
            //피해 적용
            UGameplayStatics::ApplyDamage(CommonZombie, 10, GetController(), this, UShoveDamageType::StaticClass());
        }
    }
}

void ASurvivor::DestroyShoveCylinder()
{
	if (ShoveCollisionCylinder)
	{
		ShoveCollisionCylinder->DestroyComponent();
		ShoveCollisionCylinder = nullptr;
	}
}

void ASurvivor::spawnShoveCylinder()
{
	if (ShoveCollisionCylinder)
	{
		ShoveCollisionCylinder->DestroyComponent();
		ShoveCollisionCylinder = nullptr;
	}

	//Static Mesh가 정상적으로 로드되었는지 확인
	if (!CylinderMesh)
	{
		return;
	}

	//새로운 Collision Cylinder 생성
	ShoveCollisionCylinder = NewObject<UStaticMeshComponent>(this);
	if (!ShoveCollisionCylinder)
	{
		return;
	}

	ShoveCollisionCylinder->RegisterComponent();
	ShoveCollisionCylinder->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	ShoveCollisionCylinder->SetGenerateOverlapEvents(true);
	ShoveCollisionCylinder->OnComponentBeginOverlap.AddDynamic(this, &ASurvivor::OnShoveOverlap);
	ShoveCollisionCylinder->AttachToComponent(GetRootComponent(), FAttachmentTransformRules::KeepWorldTransform);
	ShoveCollisionCylinder->SetStaticMesh(CylinderMesh);

	//위치 및 크기 설정
	FVector StartLocation = FirstCameraComp->GetComponentLocation();
	FVector CylinderLocation = FVector(StartLocation.X, StartLocation.Y, StartLocation.Z-15);
	ShoveCollisionCylinder->SetWorldLocation(CylinderLocation);
	ShoveCollisionCylinder->SetWorldScale3D(FVector(3, 3, 0.2));
	
	FRotator CameraRotation = FirstCameraComp->GetComponentRotation();
	ShoveCollisionCylinder->SetWorldRotation(CameraRotation);

	ShoveCollisionCylinder->SetVisibility(false);
	GetWorld()->GetTimerManager().SetTimer(CylinderTimerHandle, this, &ASurvivor::DestroyShoveCylinder, 0.1f, false);
}


//무기 슬롯 설정 (1,2,3,4,5(아직없고),6번(콜라) 키 바인딩)
void ASurvivor::EquipPrimaryWeapon(const struct FInputActionValue& InputValue)
{
	SwitchWeaponSlot(EWeaponType::Primary);
}

void ASurvivor::EquipSecondaryWeapon(const struct FInputActionValue& InputValue)
{
	SwitchWeaponSlot(EWeaponType::Secondary);
}

void ASurvivor::EquipMeleeWeapon(const struct FInputActionValue& InputValue)
{
	SwitchWeaponSlot(EWeaponType::Melee);
}

void ASurvivor::EquipHandleObject(const struct FInputActionValue& InputValue)
{
	SwitchWeaponSlot(EWeaponType::HandleObject);
}

void ASurvivor::EquipCokeBox(const struct FInputActionValue& InputValue)
{
	SwitchWeaponSlot(EWeaponType::CokeDelivery);
}


//무기 슬롯값 애니메이션 구분하기
int32 ASurvivor::GetCurrentWeaponSlotIndex() const
{
	// 만약, 무기를 들고 있지 않다면 아무것도 하지 않는다.
	if (nullptr == CurrentWeapon || false == CurrentWeaponSlot.IsSet())
	{
		return -1;
	}

	const FWeaponData& CurrentWeaponData = CurrentWeaponSlot.GetValue();

	if (CurrentWeaponData.WeaponName == EWeaponType::Primary)
	{
		return 0; // Primary 슬롯
	}
	else if (CurrentWeaponData.WeaponName == EWeaponType::Secondary)
	{
		return 1; // Secondary 슬롯
	}
	else if (CurrentWeaponData.WeaponName == EWeaponType::Melee)
	{
		return 2; // Melee 슬롯
	}
	else if (CurrentWeaponData.WeaponName == EWeaponType::HandleObject)
	{
		return 3; // Handle 슬롯
	}
	else if (CurrentWeaponData.WeaponName == EWeaponType::CokeDelivery)
	{
		return 5; // Coke
	}

	return -1; // 알 수 없는 슬롯
}

//무기 발견하기 (카메라 라인트레이스)
void ASurvivor::TraceForWeapon()
{
	FVector Start = FirstCameraComp->GetComponentLocation(); // 카메라 위치
	FVector ForwardVector = FirstCameraComp->GetForwardVector(); // 카메라의 정면 방향
	FVector End = Start + (ForwardVector * 300.f); // 100cm(1m) 앞까지 탐색

	const float CapsuleRadius = 30.0f; // 캡슐의 반지름 설정
	const float CapsuleHalfHeight = 50.0f; // 캡슐의 반 높이 설정
	const float DebugLineLifetime = 2.0f;

	// ObjectType이 WorldWeapon인 물체만 감지하고 싶다.
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_GameTraceChannel1));
	// 현재 자신이 가지고 있는 물체는 제외하고 싶다.
	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(CurrentWeapon);
	FHitResult OutHit;
	const bool bHit = UKismetSystemLibrary::CapsuleTraceSingleForObjects(
		GetWorld(),
		Start,
		End,
		CapsuleRadius,
		CapsuleHalfHeight,
		ObjectTypes,
		false,
		ActorsToIgnore,
		EDrawDebugTrace::None,
		OutHit,
		true
	);

	if (FocusedWeapon)
	{
		FocusedWeapon->ClearOverlayMaterial();
	}
	if (FocusedItem)
	{
		FocusedItem->ClearOverlayMaterial();
	}
	
	if (bHit)
	{
		// UE_LOG(LogTemp, Warning, TEXT("%s"), *OutHit.GetActor()->GetName());
		AActor* HitActor = OutHit.GetActor();
		
		AWeaponBase* HitWeapon = Cast<AWeaponBase>(OutHit.GetActor()); // 무기인지 확인
		AItemBase* HitItem = Cast<AItemBase>(OutHit.GetActor()); // 아이템인지 확인

		if (HitWeapon)
		{
			FocusedWeapon = HitWeapon; // 감지한 무기를 저장
			FocusedWeapon->SetOverlayMaterial(OverlayMaterial);
			FocusedItem = nullptr;
		}
		else if (HitItem)
		{
			FocusedItem = HitItem;
			FocusedItem->SetOverlayMaterial(OverlayMaterial);
			FocusedWeapon = nullptr;
		}
		else
		{
			FocusedWeapon = nullptr; // 무기가 아니면 초기화
			FocusedItem = nullptr;
		}
	}
	else
	{
		FocusedWeapon = nullptr; //감지된게 없으면 초기화
		FocusedItem = nullptr;
	}
}

//무기줍기 (E) 
void ASurvivor::PickUpWeapon_Input(const FInputActionValue& Value)
{
	if (FocusedWeapon) // 플레이어가 바라보고 있는 무기가 있다면
	{
		PickUpWeapon(FocusedWeapon);
	}
	if (FocusedItem)
	{
		FocusedItem->Interaction();
	}
}

void ASurvivor::PickUpWeapon(AWeaponBase* NewWeapon)
{
	switch (NewWeapon->SlotType)
	{
	case EWeaponType::Primary:
		PrimaryWeaponSlot = NewWeapon->WeaponData;
		break;
	case EWeaponType::Secondary:
		SecondaryWeaponSlot = NewWeapon->WeaponData;
		break;
	case EWeaponType::Melee:
		MeleeWeaponSlot = NewWeapon->WeaponData;
		bIsThrown = false;
		break;
	case EWeaponType::HandleObject:
		HandleObjectSlot = NewWeapon->WeaponData;
		break;
	case EWeaponType::CokeDelivery:
		// 코크 배달 무기로 전환하는 경우, 현재 무기 타입 저장
		beforeCokeWeapon = CurrentWeapon->WeaponData.WeaponName; // 또는 접근 방식에 맞게 수정
		CokeBoxSlot = NewWeapon->WeaponData;
		break;
	default:
		UE_LOG(LogTemp, Warning, TEXT("무기 모르겠어용"));
		return;
	}

	if (CurrentWeapon) // 이미 무기가 있다면 교체
	{
		UnequipWeapon();
	}
	
	EquipWeapon(NewWeapon);
}

void ASurvivor::SwitchWeaponSlot(EWeaponType SlotType)
{
	HoldTime = 0;
	// 현재 무기를 들고 있고,
	// TODO:콜라가 아니라면 무기를 장착 해제해라
	bool bIsNotCoke = CurrentWeapon->SlotType != EWeaponType::CokeDelivery;
	if (CurrentWeapon)
	{
		UnequipWeapon();
	}

	// TargetSlot에 있는 무기를 장착해라.
	EquipWeapon(Inventory[SlotType]);
}

void ASurvivor::ReturnToPreviousWeapon()
{
	if (CurrentWeapon && CurrentWeapon->WeaponData.WeaponName == EWeaponType::CokeDelivery)
	{
		SwitchWeaponSlot(beforeCokeWeapon);
	}
}

//무기 장착
void ASurvivor::EquipWeapon(AWeaponBase* Weapon)
{
	if (!Weapon || !Weapon->WeaponData.WeaponFactory) return;
	
	// CurrentWeapon이 유효한지 확인
	if (!CurrentWeapon)
	{
		UE_LOG(LogTemp, Warning, TEXT("무기를 들고 있지 않은 상태에서 새로 무기를 장착"));
	}
	else
	{
		// WeaponName 값을 로그로 출력
		UE_LOG(LogTemp, Warning, TEXT("손에 들고 있었던 무기의 타입번호 : %d"), static_cast<int32>(CurrentWeapon->WeaponData.WeaponName));
		UE_LOG(LogTemp, Warning, TEXT("새롭게 들 무기의 타입번호 : %d"), static_cast<int32>(Weapon->WeaponData.WeaponName));

		// 기존 무기가 장착하고 싶은 무기와 같은 슬롯에 있다면 버리기
		if (CurrentWeapon->WeaponData.WeaponName == Weapon->WeaponData.WeaponName)
		{
			UE_LOG(LogTemp, Warning, TEXT("손에 들고 있었던 무기와 장착 할 무기의 슬롯이 같으므로 이전 무기를 버린다."));
			DropWeapon();
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("손에 들고 있었던 무기와 장착 할 무기의 슬롯이 다르다."));
		}
	}

	Weapon->SetEquipped(true);
	Weapon->SetActorHiddenInGame(false);
	CurrentWeapon = Weapon;
	Weapon->Root->SetCollisionProfileName(TEXT("EquipWeapon"));
	CurrentWeaponSlot = Weapon->WeaponData;

	// 적합한 인벤토리 슬롯에 무기를 넣고 싶다.
	Inventory[Weapon->SlotType] = Weapon;
	// UI도 업데이트 하고 싶다.
	SurvivorMainUI->WeaponSlot->UpdateSlot(CurrentWeapon, Inventory);
	
	// 무기를 캐릭터의 소켓에 부착하고 싶다.
	if (Arms)
	{
		if (Weapon->SlotType == EWeaponType::CokeDelivery)
		{
			// 만약, 콜라라면 더 어울리는 소켓에 붙이고 싶다.
			CurrentWeapon->AttachToComponent(Arms, FAttachmentTransformRules::KeepRelativeTransform, "HandleSocket");
		}
		else
		{
			// 그렇지 않다면 일반적인 무기 소켓에 붙이고 싶다.
			CurrentWeapon->AttachToComponent(Arms, FAttachmentTransformRules::KeepRelativeTransform, "WeaponSocket");
		}
		CurrentWeapon->SetActorRelativeRotation(FRotator(0, 0, 0));
		CurrentWeapon->SetActorRelativeLocation(FVector(0, 0, 0));
	}
	
	UAnimInstance* AnimInst = Arms->GetAnimInstance();
	if (AnimInst)
	{
		USurvivorArmAnim* WeaponInst = Cast<USurvivorArmAnim>(AnimInst);
		if (WeaponInst)
		{
			WeaponInst->bIsEquippedWeapon=true;
		}
		
		// 마지막으로 무기를 드는 애니메이션을 재생시키고 싶다.
		if (auto* Montage = Weapon->WeaponData.WeaponDrawMontage)
		{
			AnimInst->Montage_Play(Montage);
		}
	}
}

//월드에 소환된 무기중에 false값을 가지고 있는 무기만 찾아주기
AWeaponBase* ASurvivor::FindWeaponInWorld(FWeaponData* WeaponData)
{
	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AWeaponBase::StaticClass(), FoundActors);

	for (AActor* Actor : FoundActors)
	{
		AWeaponBase* Weapon = Cast<AWeaponBase>(Actor);
		if (Weapon && Weapon->GetClass() == WeaponData->WeaponFactory->GetDefaultObject()->GetClass() && !Weapon->IsEquipped)
		{
			Weapon->Root->SetCollisionProfileName(TEXT("WorldWeapon"));
			return Weapon;
		}
	}
	return nullptr;
}

//무기 내리기
void ASurvivor::UnequipWeapon()
{
	if (CurrentWeapon)
	{
		//SKYE: 무기 프리셋 변경2
		CurrentWeapon->SetEquipped(false);
		CurrentWeapon->Root->SetCollisionProfileName(TEXT("SlotWeapon"));
		CurrentWeapon->SetActorHiddenInGame(true);
	}
	
	/*// 현재 무기 초기화
	CurrentWeapon = nullptr;
	CurrentWeaponSlot.Reset();*/

	if (UnequipMontage && GetMesh()->GetAnimInstance())
	{
		GetMesh()->GetAnimInstance()->Montage_Play(UnequipMontage);
	}

	// 애니메이션 인스턴스 업데이트
	UAnimInstance* AnimInst = Arms->GetAnimInstance();
	if (AnimInst)
	{
		USurvivorArmAnim* WeaponInst = Cast<USurvivorArmAnim>(AnimInst);
		if (WeaponInst)
		{
			WeaponInst->bIsEquippedWeapon = false;
		}
	}
}

//무기 버리기
void ASurvivor::DropWeapon()
{
	if (!CurrentWeapon)
	{
		UE_LOG(LogTemp, Warning, TEXT("CurrentWeapon nullptr"));
		return;
	}
	if (CurrentWeapon)
	{
		// 무기 부착 해제
		CurrentWeapon->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

		// 무기 상태 업데이트
		CurrentWeapon->SetEquipped(false);
		CurrentWeapon->SetActorHiddenInGame(false);
		CurrentWeapon->Root->SetCollisionProfileName(TEXT("WorldWeapon"));

		// 무기 위치 설정 (플레이어의 위치에서 약간 앞쪽으로)
		FVector DropLocation = GetActorLocation() + GetActorForwardVector() * 100.0f;
		CurrentWeapon->SetActorLocation(DropLocation);

		// 무기 회전 설정 (기본 회전값으로 설정)
		CurrentWeapon->SetActorRotation(FRotator(0, 0, 0));

		if (CurrentWeapon->WeaponData.WeaponName==EWeaponType::CokeDelivery)
		{
			ReturnToPreviousWeapon();
			return;
		}
		
		// 현재 무기 초기화
		CurrentWeapon = nullptr;
		CurrentWeaponSlot.Reset();
	}
}

void ASurvivor::DisplayIndicator(AActor* Causer)
{
	if (AttackIndicatorUIClass)
	{
		AttackIndicatorUI = CreateWidget<UUISurvivorIndicator>(GetWorld(), AttackIndicatorUIClass);
		if (AttackIndicatorUI)
		{
			AttackIndicatorUI->AddToViewport();
		}
	}
	
	AttackIndicatorUI->HitLocation = Causer->GetActorLocation();
}