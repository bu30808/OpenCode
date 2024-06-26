// Fill out your copyright notice in the Description page of Project Settings.


#include "03_Widget/01_Menu/05_Orb/OrbListButtonWidget.h"

#include "03_Widget/SimpleToolTipWidget.h"
#include "03_Widget/01_Menu/05_Orb/OrbElementListWidget.h"
#include "96_Library/ItemHelperLibrary.h"
#include "96_Library/WidgetHelperLibrary.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"
#include "Logging/StructuredLog.h"
#include "SoulLike/SoulLike.h"

void UOrbListButtonWidget::NativeConstruct()
{
	Super::NativeConstruct();
}


void UOrbListButtonWidget::OnHoveredEvent()
{
	if (OrbData != nullptr)
	{
		auto invenComp = GetOwningPlayerPawn<APlayerCharacter>()->GetInventoryComponent();
		if (UItemHelperLibrary::IsOrbFragment(OrbData->Data))
		{
			OrbData->Data = invenComp->GetFragment(OrbData->Data.UniqueID);
			SetData(OrbData);
		}
		else
		{
			OrbData->Data = invenComp->GetInventoryItem(OrbData->Data.UniqueID);
			SetData(OrbData);
		}
	}


	UGameplayStatics::PlaySound2D(this, HoverSound);
}


void UOrbListButtonWidget::SetData(UObject* Item)
{
	if (Item->IsA<UOrbData>())
	{
		OrbData = Cast<UOrbData>(Item);

		const auto& OrbItem = OrbData->Data;
		if (OrbItem.GetItemInformation() == nullptr)
		{
			UE_LOGFMT(LogTemp, Error, "아이템 액터가 널포인터입니다!! 더 이상 진행할 수 없습니다!!");
			return;
		}
		Image->SetBrushFromSoftTexture(OrbItem.GetItemInformation()->Item_Image);

		if (UItemHelperLibrary::IsOrbFragment(OrbItem))
		{
			const auto frag = static_cast<const FOrbFragmentInformation*>(OrbItem.GetItemInformation());

			switch (frag->SlotType)
			{
			case EOrbMatrixSlotType::PHYSICAL:
				Image->SetColorAndOpacity(FLinearColor::Red);
				break;
			case EOrbMatrixSlotType::DEFENCE:
				Image->SetColorAndOpacity(FLinearColor::Gray);
				break;
			case EOrbMatrixSlotType::MAGICAL:
				Image->SetColorAndOpacity(FLinearColor::Blue);
				break;
			case EOrbMatrixSlotType::FREE:
				Image->SetColorAndOpacity(FLinearColor::Yellow);
				break;
			default: ;
			}

			UWidgetHelperLibrary::SetToolTipWidget(this, UItemHelperLibrary::GetFragmentToolTipText(OrbItem));
		}
		else
		{
			UWidgetHelperLibrary::SetToolTipWidget(
				this, UItemHelperLibrary::GetItemDetailText(
					OrbItem, GetOwningPlayerPawn<ABaseCharacter>()->GetInventoryComponent()));
		}

		if (OrbData->bIsEquipped)
		{
			Image_Equipped->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		}
		else
		{
			Image_Equipped->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void UOrbListButtonWidget::SetEquipped(bool bEquipped)
{
	if (OrbData != nullptr)
	{
		OrbData->bIsEquipped = bEquipped;
		SetData(OrbData.Get());
	}
}

void UOrbListButtonWidget::UpdateData(const FInventoryItem& OrbInfo)
{
	if (OrbData != nullptr)
	{
		OrbData->Data = OrbInfo;
	}
}

void UOrbListButtonWidget::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);
	OnHoveredEvent();
}

void UOrbListButtonWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);
}
