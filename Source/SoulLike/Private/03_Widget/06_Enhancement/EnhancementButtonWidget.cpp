// Fill out your copyright notice in the Description page of Project Settings.


#include "03_Widget/06_Enhancement/EnhancementButtonWidget.h"

#include "03_Widget/06_Enhancement/EnhancementWidget.h"
#include "03_Widget/SimpleToolTipWidget.h"
#include "03_Widget/01_Menu/00_Inventory/ItemButtonWidget.h"
#include "03_Widget/01_Menu/05_Orb/OrbListButtonWidget.h"
#include "03_Widget/99_Drag/DragAndDropOperation.h"
#include "03_Widget/99_Drag/DraggableWidget.h"
#include "96_Library/ItemHelperLibrary.h"
#include "96_Library/WidgetHelperLibrary.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"

void UEnhancementButtonWidget::SetInfoFromInventoryButton(const UItemButtonWidget* InventoryButton)
{
	const auto& item = Cast<UItemData>(InventoryButton->GetInventoryData())->InventoryItem;

	Image->SetBrushFromSoftTexture(item.GetItemInformation()->Item_Image);
	EnhancementItemType = EEnhancementItemType::FromInventory;
	ItemUniqueID = item.UniqueID;
	TextBlock_ItemName->SetText(FText::FromString(item.GetItemInformation()->Item_Name));
	
	UWidgetHelperLibrary::SetToolTipWidget(this,  UItemHelperLibrary::GetItemDetailString(
		item, GetOwningPlayerPawn<ABaseCharacter>()->GetInventoryComponent()));

	EnhancementWidget->SetEnhancementInfo(item);
}

void UEnhancementButtonWidget::SetInfoFromOrbButton(UOrbListButtonWidget* OrbListButton)
{
	const auto& item = OrbListButton->GetOrbItem();
	Image->SetBrushFromSoftTexture(item.GetItemInformation()->Item_Image);
	if (UItemHelperLibrary::IsOrbCore(item))
	{
		EnhancementItemType = EEnhancementItemType::FromInventory;
	}
	else
	{
		EnhancementItemType = EEnhancementItemType::FromOrbFragmentHandler;
	}

	ItemUniqueID = item.UniqueID;
	TextBlock_ItemName->SetText(FText::FromString(item.GetItemInformation()->Item_Name));
	
	UWidgetHelperLibrary::SetToolTipWidget(this,  UItemHelperLibrary::GetItemDetailString(
		item, GetOwningPlayerPawn<ABaseCharacter>()->GetInventoryComponent()));


	EnhancementWidget->SetEnhancementInfo(item);
}

void UEnhancementButtonWidget::Clear()
{
	Image->SetBrushFromSoftTexture(DefaultImage);
	ItemUniqueID = FGuid();
	EnhancementItemType = EEnhancementItemType::None;
	TextBlock_ItemName->SetText(FText::FromString(TEXT("강화할 아이템을 올려주세요")));
	if (EnhancementWidget.IsValid())
	{
		EnhancementWidget->VerticalBox_Materials->SetVisibility(ESlateVisibility::Hidden);
	}
	SetDefaultToolTip();
}

bool UEnhancementButtonWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent,
                                            UDragDropOperation* InOperation)
{
	if (auto oper = Cast<UDragAndDropOperation>(InOperation))
	{
		if (oper->DraggedWidget.IsValid())
		{
			oper->DraggedWidget->SetRenderOpacity(oper->OriginalOpacity);
			oper->DraggedWidget->SetVisibility(ESlateVisibility::Visible);
			//드롭된 버튼이 인벤토리 버튼일 경우
			if (oper->DraggedWidget->IsA<UItemButtonWidget>())
			{
				SetInfoFromInventoryButton(Cast<UItemButtonWidget>(oper->DraggedWidget));
				return true;
			}


			//드롭된 버튼이 오브관련 버튼인 경우
			if (oper->DraggedWidget->IsA<UOrbListButtonWidget>())
			{
				SetInfoFromOrbButton(Cast<UOrbListButtonWidget>(oper->DraggedWidget));
				return true;
			}
		}
	}

	return false;
}

FReply UEnhancementButtonWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		Clear();
		return FReply::Handled();
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void UEnhancementButtonWidget::SetDefaultToolTip()
{
	const FString msg = TEXT("강화할 아이템을 올려주세요");
	if (GetToolTip() == nullptr)
	{
		SetToolTip(UWidgetHelperLibrary::GetSimpleToolTipWidget(GetOwningPlayer(), msg));
	}
	else
	{
		Cast<USimpleToolTipWidget>(GetToolTip())->SetDescriptionText(msg);
	}
}

void UEnhancementButtonWidget::NativeConstruct()
{
	Super::NativeConstruct();

	Clear();
	SetDefaultToolTip();
}

void UEnhancementButtonWidget::SetOwnerWidget(UEnhancementWidget* E_Widget)
{
	EnhancementWidget = E_Widget;
}

void UEnhancementButtonWidget::OverrideToolTip(const FInventoryItem& RefreshedItem)
{
	UWidgetHelperLibrary::SetToolTipWidget(this,  UItemHelperLibrary::GetItemDetailString(
	RefreshedItem, GetOwningPlayerPawn<ABaseCharacter>()->GetInventoryComponent()));
}