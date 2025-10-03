// Fill out your copyright notice in the Description page of Project Settings.


#include "CommonZombieAIController.h"

#include "BrainComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "Perception/AIPerceptionComponent.h"


// Sets default values
ACommonZombieAIController::ACommonZombieAIController()
{
	PrimaryActorTick.bCanEverTick = true;
	SetTickableWhenPaused(true);
	SetActorTickInterval(0.1f);
	bStartAILogicOnPossess = true;
	bCanBeInCluster = true;
}

// Called when the game starts or when spawned
void ACommonZombieAIController::BeginPlay()
{
	Super::BeginPlay();
	SetFolderPath(TEXT("Zombie/CommonZombie/AIController"));
}

// Called every frame
void ACommonZombieAIController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
}

void ACommonZombieAIController::SetActivate(bool bNewActive)
{
	if (bIsActive != bNewActive)
	{
		bIsActive = bNewActive;
		
		// BrainComponent 활성/비활성화
		if (BrainComponent)
		{
			if (bIsActive)
			{
				BrainComponent->RestartLogic();
			}
			else
			{
				BrainComponent->StopLogic("Pawn is inactive");
			}
		}
		
		// AIPerception 활성/비활성화
		if (UAIPerceptionComponent* PerceptionComp = GetAIPerceptionComponent())
		{
			PerceptionComp->SetActive(bIsActive);
		}
		
		// PathFollowingComponent 활성/비활성화
		if (UPathFollowingComponent* PathFollowingComp = GetPathFollowingComponent())
		{
			PathFollowingComp->SetActive(bIsActive);
		}

		SetActorTickEnabled(bIsActive);
	}
}