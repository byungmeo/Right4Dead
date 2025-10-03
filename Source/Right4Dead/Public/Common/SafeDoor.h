// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ItemBase.h"
#include "GameFramework/Actor.h"
#include "SafeDoor.generated.h"

UCLASS()
class RIGHT4DEAD_API ASafeDoor : public AItemBase
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ASafeDoor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<USceneComponent> Root = nullptr;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UStaticMeshComponent> Frame = nullptr;
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	TObjectPtr<USceneComponent> Hinge = nullptr;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UStaticMeshComponent> Door = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	bool bIsOpen = true;
	
	virtual void Interaction() override;
	virtual void SetOverlayMaterial(UMaterialInterface* MyOverlayMaterial) override;
	virtual void ClearOverlayMaterial() override;

	UFUNCTION(CallInEditor, BlueprintImplementableEvent, Category = "Debugging")
	void InteractionDoor();
};
