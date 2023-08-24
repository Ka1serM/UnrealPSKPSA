// Fill out your copyright notice in the Description page of Project Settings.

#include "Widgets/SPSAImportOption.h"
#include "SPrimaryButton.h"
#include "Widgets/PSAImportOptions.h"
#include "SlateOptMacros.h"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SPSAImportOption::Construct(const FArguments& InArgs)
{
	WidgetWindow = InArgs._WidgetWindow;
	FPropertyEditorModule& EditModule = FModuleManager::Get().GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FDetailsViewArgs DetailsViewArgs;
	DetailsViewArgs.bAllowSearch = false;
	DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
	DetailsViewArgs.bHideSelectionTip = true;
	TSharedRef<IDetailsView> Details = EditModule.CreateDetailView(DetailsViewArgs);
	EditModule.CreatePropertyTable();
	UObject* Container = NewObject<UPSAImportOptions>();
	Stun = Cast<UPSAImportOptions>(Container);
	Details->SetObject(Container);
	Details->SetEnabled(true);

	this->ChildSlot
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::Get().GetBrush(TEXT("Brushes.Panel")))
		.Padding(10)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SBox)
			.Padding(FMargin(3))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
		]
		]

	// Data row struct
	// Curve interpolation
	// Details panel
	+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(2)
		[
			SNew(SBox)
			.WidthOverride(400)
		[
			Details
		]
		]
	+SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
		    .HAlign(HAlign_Right)
		    .Padding(2)
		[
			SNew(SPrimaryButton)
			.Text(FText::FromString(TEXT("Apply")))
		    .OnClicked(this, &SPSAImportOption::OnImport)
		]
	+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2)
		[
			SNew(SButton)
			.Text(FText::FromString(TEXT("Apply to All")))
		.OnClicked(this, &SPSAImportOption::OnImportAll)
		]
	+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2)
		[
			SNew(SButton)
			.Text(FText::FromString(TEXT("Cancel")))
		.OnClicked(this, &SPSAImportOption::OnCancel)
		]
		]
		]
	// Apply/Apply to All/Cancel
		];


}
bool SPSAImportOption::ShouldImport()
{
	return UserDlgResponse == EPSAImportOptionDlgResponse::Import;
}
bool SPSAImportOption::ShouldImportAll()
{
	return UserDlgResponse == EPSAImportOptionDlgResponse::ImportAll;
}
FReply SPSAImportOption::OnImportAll()
{
	UserDlgResponse = EPSAImportOptionDlgResponse::ImportAll;
	return HandleImport();
}
FReply SPSAImportOption::OnImport()
{
	UserDlgResponse = EPSAImportOptionDlgResponse::Import;
	return HandleImport();
}
FReply SPSAImportOption::OnCancel()
{
	UserDlgResponse = EPSAImportOptionDlgResponse::Cancel;
	return FReply::Handled();
}
FReply SPSAImportOption::HandleImport()
{
	if (WidgetWindow.IsValid())
	{
		WidgetWindow.Pin()->RequestDestroyWindow();
	}
	return FReply::Handled();
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION
