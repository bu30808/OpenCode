// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "04_Item/ItemActor.h"
#include "04_Item/05_Ability/AbilityItemActor.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "ItemManagerSubsystem.generated.h"

/**
 * 
 */
UCLASS()
class SOULLIKE_API UItemManagerSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

private:
	UPROPERTY(EditAnywhere)
	class UDataTable* ConsumeItemTable;
	UPROPERTY(EditAnywhere)
	class UDataTable* SpiritItemTable;
	UPROPERTY(EditAnywhere)
	class UDataTable* ArmorItemTable;
	UPROPERTY(EditAnywhere)
	class UDataTable* RingItemTable;
	UPROPERTY(EditAnywhere)
	class UDataTable* OrbCoreItemTable;
	UPROPERTY(EditAnywhere)
	class UDataTable* OrbFragmentItemTable;
	UPROPERTY(EditAnywhere)
	class UDataTable* EnhancementItemTable;
	UPROPERTY(EditAnywhere)
	class UDataTable* PotionItemTable;
	UPROPERTY(EditAnywhere)
	class UDataTable* KeyItemTable;
	UPROPERTY(EditAnywhere)
	class UDataTable* FragmentSlotOpenMaterialTable;
	UPROPERTY(EditAnywhere)
	class UDataTable* StuffItemTable;
	UPROPERTY(EditAnywhere)
	class UDataTable* AbilityItemTable;
	
public:
	UItemManagerSubsystem();

	/*virtual void Initialize(FSubsystemCollectionBase& Collection) override;*/
	//아이템 기본정보
	const FItemInformation* GetConsumeItemInformation(FGameplayTag ItemTag) const;
	const FItemInformation* GetSpiritItemInformation(FGameplayTag ItemTag) const;
	const FItemInformation* GetArmorItemInformation(FGameplayTag ItemTag) const;
	const FItemInformation* GetRingItemInformation(FGameplayTag ItemTag) const;
	const FItemInformation* GetOrbCoreItemInformation(FGameplayTag ItemTag) const;
	const FItemInformation* GetOrbFragmentItemInformation(FGameplayTag ItemTag) const;
	const FItemInformation* GetEnhancementItemInformation(FGameplayTag ItemTag) const;
	const FItemInformation* GetPotionItemInformation(FGameplayTag ItemTag) const;
	const FItemInformation* GetKeyItemInformation(FGameplayTag ItemTag) const;
	const FItemInformation* GetFragmentSlotOpenMaterialItemInformation(FGameplayTag ItemTag) const;
	const FItemInformation* GetStuffItemInformation(FGameplayTag ItemTag) const;
	const FAbilityItemInformation* GetAbilityInformation(FGameplayTag ItemTag) const;
};
