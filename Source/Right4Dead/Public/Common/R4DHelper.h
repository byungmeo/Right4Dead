// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "R4DHelper.generated.h"

/**
 * 
 */
UCLASS()
class RIGHT4DEAD_API UR4DHelper : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintPure)
	static FName GetSignatureBone(const USkeletalMeshComponent* SkeletalMeshComp, const FName& BoneName);
	
    UFUNCTION(BlueprintPure)
    static bool IsChildBone(const USkeletalMeshComponent* SkeletalMeshComp, const FName& BoneName, const FName& TargetBoneName);
};
