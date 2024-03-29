// Fill out your copyright notice in the Description page of Project Settings.


#include "00_Character/03_Monster/03_AI/00_Common/00_Task/BTTask_PlayAnimMontage.h"

#include "AIController.h"
#include "00_Character/BaseCharacter.h"
#include "00_Character/01_Component/AttributeComponent.h"
#include "00_Character/03_Monster/00_Controller/MonsterAIController.h"
#include "Logging/StructuredLog.h"

//가끔 탈출하지 못함

UBTTask_PlayAnimMontage::UBTTask_PlayAnimMontage()
{
	bCreateNodeInstance = true;
	bNotifyTaskFinished = true;
	NodeName = "PlayMontage";
	/*bNotifyTick = true;*/
}

EBTNodeResult::Type UBTTask_PlayAnimMontage::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	if (MontageToPlay)
	{
		AIController = OwnerComp.GetAIOwner();
		
		if (auto monster = OwnerComp.GetAIOwner()->GetPawn<ABaseCharacter>())
		{
			if (!monster->GetIsTriggeredHitAnimationExitEvent())
			{
				UE_LOGFMT(LogAICon, Log, "{0} {1} : OnTriggerHitAnimationExit이벤트가 발동하지 않은 상태임으로 강제로 브로드케스트 합니다.",
				          __FUNCTION__, __LINE__);
				monster->OnTriggerHitAnimationExit.Broadcast(monster, nullptr);
			}

			if (auto instance = monster->GetMesh()->GetAnimInstance())
			{
		
				if (!bEnableRootMotion)
				{
					instance->RootMotionMode = ERootMotionMode::IgnoreRootMotion;
				}
	
				float playSpeed = 1.f;
				if (bUseCustomPlayRate)
				{
					playSpeed = PlaySpeed;
				}
				else
				{
					playSpeed = monster->GetAttributeComponent()->GetActionSpeed();
				}

				if (bNonBlocking)
				{
					instance->Montage_Play(MontageToPlay, playSpeed);
					return EBTNodeResult::Succeeded;
				}
				
				instance->OnMontageBlendingOut.AddUniqueDynamic(this, &UBTTask_PlayAnimMontage::OnMontageBlendingOutEvent);
				instance->Montage_Play(MontageToPlay, playSpeed);

				return EBTNodeResult::InProgress;
			}
			else
			{
				UE_LOGFMT(LogAICon, Error, "인스턴스를 가져올 수 없어요!!");
			}
		}
	}
	UE_LOGFMT(LogAICon, Error, "플레이할 몽타주가 없어요!");
	UKismetSystemLibrary::PrintString(AIController,FString::Printf(TEXT("몽타주 재생에 실패했음!!!!!!!!!!!!!!!!!!")));
	return EBTNodeResult::Failed;
}

void UBTTask_PlayAnimMontage::OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory,
                                             EBTNodeResult::Type TaskResult)
{
	Super::OnTaskFinished(OwnerComp, NodeMemory, TaskResult);
	if (auto aiCon = OwnerComp.GetAIOwner())
	{
		// Root Motion 모드를 복구
		if (auto pawn = aiCon->GetPawn<ABaseCharacter>())
		{
			if (auto instance = pawn->GetMesh()->GetAnimInstance())
			{
				instance->RootMotionMode = ERootMotionMode::RootMotionFromMontagesOnly;
			}
		}
	}
}

void UBTTask_PlayAnimMontage::OnMontageBlendingOutEvent(UAnimMontage* Montage, bool bInterrupted)
{
	if (MontageToPlay == Montage)
	{
		if(AIController!=nullptr)
		{
			UKismetSystemLibrary::PrintString(AIController,FString::Printf(TEXT("%s 가 종료되었습니다."),*Montage->GetName()));
			if(auto pawn = AIController->GetPawn<ABaseCharacter>())
			{
				if (auto instance = pawn->GetMesh()->GetAnimInstance())
				{
					instance->OnMontageBlendingOut.RemoveAll(this);
					FinishLatentTask(*Cast<UBehaviorTreeComponent>(AIController->GetBrainComponent()), EBTNodeResult::Succeeded);
				}
			}
		}
	}
}
