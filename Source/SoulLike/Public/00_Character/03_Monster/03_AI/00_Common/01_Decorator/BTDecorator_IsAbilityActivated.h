// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Decorators/BTDecorator_BlackboardBase.h"
#include "BTDecorator_IsAbilityActivated.generated.h"

/**
 * 
 */
UCLASS()
class SOULLIKE_API UBTDecorator_IsAbilityActivated : public UBTDecorator_BlackboardBase
{
	GENERATED_BODY()
	UBTDecorator_IsAbilityActivated();
	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;

public:
	UPROPERTY(EditAnywhere)
	FGameplayTag TagToCheck;
};
