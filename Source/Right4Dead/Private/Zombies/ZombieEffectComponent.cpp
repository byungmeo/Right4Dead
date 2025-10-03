// Fill out your copyright notice in the Description page of Project Settings.


#include "ZombieEffectComponent.h"

#include "ZombieBase.h"


// Sets default values for this component's properties
UZombieEffectComponent::UZombieEffectComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}


// Called when the game starts
void UZombieEffectComponent::BeginPlay()
{
	Super::BeginPlay();

	Owner = Cast<AZombieBase>(GetOwner());
	if (Owner)
	{
		Owner->OnBulletHit.AddDynamic(this, &UZombieEffectComponent::OnBulletHit);
		Owner->OnMeleeHit.AddDynamic(this, &UZombieEffectComponent::OnMeleeHit);
		Owner->OnDead.AddDynamic(this, &UZombieEffectComponent::OnDead);
	}
}

