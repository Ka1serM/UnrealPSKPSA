// Fill out your copyright notice in the Description page of Project Settings.

#include "Factories/PSAFactory.h"

#include "ComponentReregisterContext.h"
#include "Widgets/PSAImportOptions.h"
#include "Readers/PSAReader.h"
#include "Widgets/SPSAImportOption.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Interfaces/IMainFrameModule.h"
#include "Misc/FeedbackContext.h"
#include "Misc/ScopedSlowTask.h"
#include "Utils/ActorXUtils.h"

/* UTextAssetFactory structors
 *****************************************************************************/

UPSAFactory::UPSAFactory( const FObjectInitializer& ObjectInitializer )
	: Super(ObjectInitializer)
{
	Formats.Add(TEXT("psa;PSA Animation File"));
	SupportedClass = UAnimSequence::StaticClass();
	bCreateNew = false;
	bEditorImport = true;
    SettingsImporter = CreateDefaultSubobject<UPSAImportOptions>(TEXT("Anim Options"));
}

/* UFactory overrides
 *****************************************************************************/

UObject* UPSAFactory::FactoryCreateFile(UClass* Class, UObject* Parent, FName Name, EObjectFlags Flags, const FString& Filename, const TCHAR* Params, FFeedbackContext* Warn, bool& bOutOperationCanceled)
{

	/* Ui
	*****************************************************************************/
	
    FScopedSlowTask SlowTask(5, NSLOCTEXT("PSAFactory", "BeginReadPSAFile", "Opening PSA file."), true);
    if (Warn->GetScopeStack().Num() == 0)
    {
        // We only display this slow task if there is no parent slowtask, because otherwise it is redundant and doesn't display any relevant information on the progress.
        // It is primarly used to regroup all the smaller import sub-tasks for a smoother progression.
        SlowTask.MakeDialog(true);
    }
    SlowTask.EnterProgressFrame(0);

    // picker
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
            .Title(FText::FromString(TEXT("PSA Import Options")))
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

	/* UFactory overrides
	 *****************************************************************************/

	auto Data = PSAReader(Filename);
	if (!Data.Read()) return nullptr;

	const auto AnimSequence = FActorXUtils::LocalCreate<UAnimSequence>(UAnimSequence::StaticClass(), Parent,  Name.ToString(), Flags);

	USkeleton* Skeleton = SettingsImporter->Skeleton;
	AnimSequence->SetSkeleton(Skeleton);
	
	AnimSequence->GetController().OpenBracket(FText::FromString("Importing PSA Animation"));
	AnimSequence->GetController().InitializeModel();
	AnimSequence->ResetAnimation();
	
	auto Info = Data.AnimInfo;
	
	AnimSequence->GetController().SetFrameRate(FFrameRate(Info.AnimRate, 1));
	AnimSequence->GetController().SetNumberOfFrames(FFrameNumber(Info.NumRawFrames));
	
	FScopedSlowTask ImportTask(Data.Bones.Num(), FText::FromString("Importing PSA Animation"));
	ImportTask.MakeDialog(false);
	for (auto BoneIndex = 0; BoneIndex < Data.Bones.Num(); BoneIndex++)
	{
		auto Bone = Data.Bones[BoneIndex];
		auto BoneName = FName(Bone.Name);
		
		ImportTask.DefaultMessage = FText::FromString(FString::Printf(TEXT("Bone %s: %d/%d"), *BoneName.ToString(), BoneIndex+1, Data.Bones.Num()));
		ImportTask.EnterProgressFrame();
		
		TArray<FVector3f> PositionalKeys;
		TArray<FQuat4f> RotationalKeys;
		TArray<FVector3f> ScaleKeys;
		for (auto Frame = 0; Frame < Info.NumRawFrames; Frame++)
		{
			auto KeyIndex = BoneIndex + Frame * Data.Bones.Num();
			auto AnimKey = Data.AnimKeys[KeyIndex];

			UE_LOG(LogTemp, Warning, TEXT(" Frame %s"), *AnimKey.Position.ToString());
			PositionalKeys.Add(FVector3f(AnimKey.Position.X, -AnimKey.Position.Y, AnimKey.Position.Z));
			RotationalKeys.Add(FQuat4f(-AnimKey.Orientation.X, AnimKey.Orientation.Y, -AnimKey.Orientation.Z, (BoneIndex == 0) ? AnimKey.Orientation.W : -AnimKey.Orientation.W).GetNormalized());
			ScaleKeys.Add(Data.bHasScaleKeys ? Data.ScaleKeys[KeyIndex].ScaleVector : FVector3f::OneVector);

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
	AnimSequence->MarkPackageDirty();

	FGlobalComponentReregisterContext RecreateComponents;
	
	return AnimSequence;
}


