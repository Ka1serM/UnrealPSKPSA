// Fill out your copyright notice in the Description page of Project Settings.


#include "PSAFactory.h"
#include "PSAImportOptions.h"
#include "PSAReader.h"
#include "SPSAImportOption.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Misc/ScopedSlowTask.h"



UObject* UPSAFactory::Import(const FString Filename, UObject* Parent, const FName Name, const EObjectFlags Flags)
{
	auto Psa = PSAReader(Filename);
	if (!Psa.Read()) return nullptr;
	
	if (SettingsImporter->bInitialized == false)
	{
		TSharedPtr<SPSAImportOption> ImportOptionsWindow;
		TSharedPtr<SWindow> ParentWindow;
		if (FModuleManager::Get().IsModuleLoaded("MainFrame"))
		{
			IMainFrameModule& MainFrame = FModuleManager::LoadModuleChecked<IMainFrameModule>("MainFrame");
			ParentWindow = MainFrame.GetParentWindow();
		}
		TSharedRef<SWindow> Window = SNew(SWindow)
			.Title(LOCTEXT("DatasmithImportSettingsTitle", "PSA Import Options"))
			.SizingRule(ESizingRule::Autosized);
		Window->SetContent
		(
			SAssignNew(ImportOptionsWindow, SPSAImportOption)
			.WidgetWindow(Window)
		);
		SettingsImporter = ImportOptionsWindow.Get()->Stun;
		FSlateApplication::Get().AddModalWindow(Window, ParentWindow, false);
		bImport = ImportOptionsWindow.Get()->ShouldImport();
		bImportAll = ImportOptionsWindow.Get()->ShouldImportAll();
		SettingsImporter->bInitialized = true;
	}

	UAnimSequence* AnimSequence = NewObject<UAnimSequence>(Parent, UAnimSequence::StaticClass(), Name, Flags);
	USkeleton* Skeleton = SettingsImporter->Skeleton;
	AnimSequence->SetSkeleton(Skeleton);
	
	AnimSequence->GetController().OpenBracket(LOCTEXT("ImportAnimation_Bracket", "Importing Animation"));
	AnimSequence->GetController().InitializeModel();
	AnimSequence->ResetAnimation();
	
	auto Info = Psa.AnimInfo;
	
	AnimSequence->GetController().SetFrameRate(FFrameRate(Info.AnimRate, 1));
	AnimSequence->GetController().SetNumberOfFrames(FFrameNumber(Info.NumRawFrames));

	
	FScopedSlowTask ImportTask(Psa.Bones.Num(), FText::FromString("Importing Anim"));
	ImportTask.MakeDialog(false);
	for (auto BoneIndex = 0; BoneIndex < Psa.Bones.Num(); BoneIndex++)
	{
		auto Bone = Psa.Bones[BoneIndex];
		auto BoneName = FName(Bone.Name);
		ImportTask.DefaultMessage = FText::FromString(FString::Printf(TEXT("Bone %s: %d/%d"), *BoneName.ToString(), BoneIndex+1, Psa.Bones.Num()));
		ImportTask.EnterProgressFrame();
		
		TArray<FVector3f> PositionalKeys;
		TArray<FQuat4f> RotationalKeys;
		TArray<FVector3f> ScaleKeys;
		for (auto Frame = 0; Frame < Info.NumRawFrames; Frame++)
		{
			auto KeyIndex = BoneIndex + Frame * Psa.Bones.Num();
			auto AnimKey = Psa.AnimKeys[KeyIndex];

			UE_LOG(LogTemp, Warning, TEXT(" Position %s"), *AnimKey.Position.ToString());
			PositionalKeys.Add(FVector3f(AnimKey.Position.X, -AnimKey.Position.Y, AnimKey.Position.Z));
			RotationalKeys.Add(FQuat4f(-AnimKey.Orientation.X, AnimKey.Orientation.Y, -AnimKey.Orientation.Z, (BoneIndex == 0) ? AnimKey.Orientation.W : -AnimKey.Orientation.W).GetNormalized());
			ScaleKeys.Add(Psa.bHasScaleKeys ? Psa.ScaleKeys[KeyIndex].ScaleVector : FVector3f::OneVector);

		}

		AnimSequence->GetController().AddBoneCurve(BoneName);
		AnimSequence->GetController().SetBoneTrackKeys(BoneName, PositionalKeys, RotationalKeys, ScaleKeys);
	}
	
	if (!bImportAll)
	{
		SettingsImporter->bInitialized = false;
	}
	
	AnimSequence->GetController().NotifyPopulated();
	AnimSequence->GetController().CloseBracket();
	AnimSequence->Modify(true);
	AnimSequence->PostEditChange();
	FAssetRegistryModule::AssetCreated(AnimSequence);
	bool bDirty = AnimSequence->MarkPackageDirty();
	
	return AnimSequence;
}


