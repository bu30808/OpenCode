// Fill out your copyright notice in the Description page of Project Settings.


#include "03_Widget/QuickSlotWidget.h"

#include "03_Widget/01_Menu/02_QuickSlot/QuickSlotButtonWidget.h"
#include "Components/HorizontalBox.h"

void UQuickSlotWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	if (UMG_QuickSlotButton)
	{
		UMG_QuickSlotButton->SetIndex(-1);
	}

	if (UMG_AbilitySlotButton)
	{
		UMG_AbilitySlotButton->SetIndex(-1);
	}
}

void UQuickSlotWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (auto invenComp = GetOwningPlayerPawn<ABaseCharacter>()->GetInventoryComponent())
	{
		invenComp->OnUpdateMainAbilityQuickSlot.AddUniqueDynamic(
			this, &UQuickSlotWidget::OnUpdateCurAbilityQuickSlotEvent);
		invenComp->OnUpdateMainConsumeQuickSlot.AddUniqueDynamic(
			this, &UQuickSlotWidget::OnUpdateCurConsumeQuickSlotEvent);
	}
}

void UQuickSlotWidget::OnUpdateCurAbilityQuickSlotEvent(const FGameplayTag& Tag, bool bRemove, int32 SelectedIndex)
{
	if (UMG_AbilitySlotButton)
	{
		if (auto data = UMG_AbilitySlotButton->GetQuickSlotData())
		{
			if (Cast<UAbilityData>(data)->AbilityInformation.AbilityTag.MatchesTagExact(Tag))
			{
				if (bRemove)
				{
					UMG_AbilitySlotButton->CleanMainQuickSlotButton();
				}
				//어빌리티는 굳이 새로고칠 필요가 없음.
			}
		}
		else
		{
			//TODO - 어빌리티 등록 기능 추가
			//UMG_AbilitySlotButton->Init()
		}
	}
}

void UQuickSlotWidget::OnUpdateCurConsumeQuickSlotEvent(const FInventoryItem& Item, bool bRemove, int32 SelectedIndex)
{
	if (UMG_QuickSlotButton)
	{
		if (bRemove)
		{
			if (auto data = UMG_QuickSlotButton->GetQuickSlotData())
			{
				if (Cast<UItemData>(data)->InventoryItem.UniqueID == Item.UniqueID)
				{
					UMG_QuickSlotButton->CleanMainQuickSlotButton();
				}
			}
		}
		else
		{
			if (auto newData = NewObject<UItemData>())
			{
				newData->InventoryItem = Item;
				UMG_QuickSlotButton->Init(newData, false, true);
			}
		}
	}
}

void UQuickSlotWidget::LoadSelectedConsumeQuickSlot(const FInventoryItem& InventoryItem)
{
	if (UMG_QuickSlotButton)
	{
		if (auto newData = NewObject<UItemData>())
		{
			newData->InventoryItem = InventoryItem;
			UMG_QuickSlotButton->Init(newData, false, true);
		}
	}
}

void UQuickSlotWidget::LoadSelectedAbilityQuickSlot(UAbilityBase* Ability)
{
	if (UMG_AbilitySlotButton)
	{
		//TODO - 어빌리티 등록 기능 추가
	}
}
