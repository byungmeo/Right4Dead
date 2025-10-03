// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CommonDoor.h"
#include "CokeDelivery.generated.h"

UCLASS()
class RIGHT4DEAD_API ACokeDelivery : public ACommonDoor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ACokeDelivery();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	bool bIsCokeDelivery = true;
	
	virtual void SetOverlayMaterial(UMaterialInterface* MyOverlayMaterial) override;
	virtual void ClearOverlayMaterial() override;
	
	virtual void Interaction() override;
	UFUNCTION(CallInEditor, BlueprintImplementableEvent, Category = "Debugging")
	void InteractionDelivery();
};
