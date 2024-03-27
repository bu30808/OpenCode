// Copyright Epic Games, Inc. All Rights Reserved.


#include "01_GameMode/SoulLikeGameMode.h"

#include "00_Character/03_Monster/BaseMonster.h"
#include "96_Library/DataLayerHelperLibrary.h"
#include "98_GameInstance/SoulLikeInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Logging/StructuredLog.h"
#include "UObject/ConstructorHelpers.h"
#include "WorldPartition/DataLayer/DataLayerAsset.h"


class UDataLayerSubsystem;
DEFINE_LOG_CATEGORY(LogSoulLikeGameMode)

ASoulLikeGameMode::ASoulLikeGameMode()
{
	
}

void ASoulLikeGameMode::AddToRespawnMonster(ABaseMonster* BaseMonster)
{
	const auto& safeName = FName(GetNameSafe(BaseMonster));

	if (!RespawnInfos.Contains(safeName))
	{
		RespawnInfos.Add(safeName);
	}
	
	UE_LOGFMT(LogMonster, Log, "몬스터 추가 : {0}", safeName);
	RespawnInfos[safeName] = FActorSave(BaseMonster,safeName,BaseMonster->GetClass(), BaseMonster->GetActorTransform());
}

void ASoulLikeGameMode::RemoveFromRespawnMonster(const ABaseMonster* BaseMonster)
{
	if(BaseMonster->IsValidLowLevel())
	{
		const auto& safeName = GetNameSafe(BaseMonster);
		RemoveFromRespawnMonsterWithSafeName(safeName);
	}

}

void ASoulLikeGameMode::RemoveFromRespawnMonsterWithSafeName(const FString& SafeName)
{

		const auto& safeName = FName(SafeName);
		if (!RespawnInfos.Contains(safeName))
		{
			return;
		}
		UE_LOGFMT(LogMonster, Log, "몬스터 제거 : {0}", safeName);
		RespawnInfos.Remove(safeName);
}

void ASoulLikeGameMode::SaveMonsterAttributeWhenUnload(const ABaseMonster* BaseMonster)
{
	const auto& safeName = FName(GetNameSafe(BaseMonster));
	if(!TemporarySavedMonsterState.Contains(safeName))
	{
		TemporarySavedMonsterState.Add(safeName,FCharacterSave());
		return;
	}

	const auto attComp = BaseMonster->GetAttributeComponent();
	const auto& attributes = attComp->GetAllAttributeNotIncludeLevelUpPoint();
	
	for(const auto& iter : attributes)
	{
		TemporarySavedMonsterState[safeName].Attributes.Add(iter.Key,*iter.Value);
	}
}

void ASoulLikeGameMode::RespawnMonsters(class APlayerCharacter* Player)
{
	for(const auto& iter : RespawnInfos)
	{
		const auto& savedData = iter.Value;

		if(!savedData.ActorPointer->IsValidLowLevel())
		{
			UE_LOGFMT(LogMonster, Warning, "유효하지 않은 포인터라 리스폰할 수 없습니다. : {0}", savedData.ActorClass->GetName());
		}else
		{
			if(savedData.ActorPointer->IsA<ABaseCharacter>())
			{
				if(Cast<ABaseCharacter>(savedData.ActorPointer)->IsDead())
				{
					UE_LOGFMT(LogMonster, Warning, "다음 몬스터를 리스폰 할 것입니다 : {0} {1}", savedData.ActorClass->GetName() ,GetNameSafe(savedData.ActorPointer));
					RemoveFromRespawnMonster(Cast<ABaseMonster>(savedData.ActorPointer));
					Cast<ABaseMonster>(savedData.ActorPointer)->Activate();
				}
				//위치만 다른 친구들의 경우, 위치랑 어트리뷰트만 초기화 해 줍니다.
				else
				{
					RemoveFromRespawnMonster(Cast<ABaseMonster>(savedData.ActorPointer));
					Cast<ABaseMonster>(savedData.ActorPointer)->Activate();
				}
			}
		}
		
	}
}

void ASoulLikeGameMode::OnDeadMonsterEvent(AActor* Who, AActor* DeadBy)
{
	if(Who)
	{
		const auto& safeName = FName(GetNameSafe(Who));
		
		if(!TemporarySavedMonsterState.Contains(safeName))
		{
			if(auto character =  Cast<ABaseCharacter>(Who))
			{
				auto attComp = character->GetAttributeComponent();
				TemporarySavedMonsterState.Add(safeName, FCharacterSave(Who,safeName,Who->GetClass(),Who->GetActorTransform(),attComp->GetAllAttributeNotIncludeLevelUpPoint(),character->GetCharacterState()));
			}
		}else
		{
			UE_LOGFMT(LogSave,Error,"이미 해당 몬스터의 상태정보가 저장되어 있습니다. : {0}",safeName);
		}

		if(!DeadBy->IsA<APlayerCharacter>())
		{
			DeadBy = UGameplayStatics::GetPlayerCharacter(this,0);
		}

		Cast<APlayerCharacter>(DeadBy)->SetPlayerStateBy(EPlayerCharacterState::Peaceful,Who);
	}
}

void ASoulLikeGameMode::RestoreMonsterState(ABaseMonster* BaseMonster)
{
	if(BaseMonster)
	{
		const auto& safeName = FName(GetNameSafe(BaseMonster));

		if(TemporarySavedMonsterState.Contains(safeName))
		{
			const auto& savedData = TemporarySavedMonsterState[safeName];
			if(savedData.CharacterState == ECharacterState::DEAD)
			{
				UE_LOGFMT(LogMonster,Log,"사망 상태로 되돌립니다 : {0}",safeName);
				BaseMonster->SetActorTransform(savedData.ActorTransform);
				BaseMonster->StopAITree();
				BaseMonster->EnableRagdoll();
				BaseMonster->RunDeactivateTimer();
			}else
			{
				const auto attComp = BaseMonster->GetAttributeComponent();
				attComp->LoadAttributeNotIncludeLevelUpPoint(savedData.Attributes, false, true);
			}
		}
		
		/*
		const auto& layers =  BaseMonster->GetDataLayerAssets();
		 if(layers.IsValidIndex(0))
		{
			const auto& fullPath = UDataLayerHelperLibrary::GetLayerFullPath(BaseMonster->GetWorld(), layers[0]);
			if(DeadMonsters.Contains(fullPath))
			{
				if(DeadMonsters[fullPath].Contains(safeName))
				{
					UE_LOGFMT(LogMonster,Log,"사망 상태로 되돌립니다 : {0}",safeName);
					BaseMonster->StopAITree();
					BaseMonster->EnableRagdoll();
					BaseMonster->RunDeactivateTimer();
					return true;
				}
			}else
			{
				UE_LOGFMT(LogMonster,Log,"{0}가 속한 {1} 레이어에 대한 정보가 없습니다.",safeName,fullPath);
			}
		}else
		{
			UE_LOGFMT(LogMonster,Log,"{0}의 레이어가 유효하지 않습니다.",safeName);
		}*/
	}
}

bool ASoulLikeGameMode::IsContainDeadList(const ABaseMonster* BaseMonster)
{
	if(BaseMonster)
	{
		const auto& safeName = FName(GetNameSafe(BaseMonster));
		
		if(TemporarySavedMonsterState.Contains(safeName))
		{
			const auto& savedData = TemporarySavedMonsterState[safeName];
			return savedData.CharacterState == ECharacterState::DEAD;
		}
		/*const auto& layers =  BaseMonster->GetDataLayerAssets();
		if(layers.IsValidIndex(0))
		{
			const auto& fullPath = UDataLayerHelperLibrary::GetLayerFullPath(BaseMonster->GetWorld(), layers[0]);
			if(DeadMonsters.Contains(fullPath))
			{
				return DeadMonsters[fullPath].Contains(safeName);
				
			}
		}*/
	}

	return false;
}

void ASoulLikeGameMode::ClearTemporarySavedMonsterData(APlayerCharacter* Player)
{
	TemporarySavedMonsterState.Empty();
}

void ASoulLikeGameMode::Respawn(const UWorld* World, const FTransform& SpawnTr, TSubclassOf<AActor> MonsterClass, const TArray<TObjectPtr<const
	                                UDataLayerAsset>>& LayerInfo)
{
	FActorSpawnParameters spawnParams;
	spawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	UE_LOGFMT(LogMonster, Warning, "다음 몬스터를 리스폰시도 합니다. : {0}",MonsterClass->GetName());
	if(auto spawnMonster =
		UGameplayStatics::FinishSpawningActor(UGameplayStatics::BeginDeferredActorSpawnFromClass(World, MonsterClass, SpawnTr,
														   ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn,nullptr,ESpawnActorScaleMethod::OverrideRootScale),SpawnTr,ESpawnActorScaleMethod::OverrideRootScale))
	{
		for(auto iter : LayerInfo)
		{
			spawnMonster->AddDataLayer(iter);
		}
		UE_LOGFMT(LogMonster, Warning, "다음 몬스터를 리스폰합니다 : {0}", GetNameSafe(spawnMonster));
	}
}
