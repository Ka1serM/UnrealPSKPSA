// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "PSKFactory.generated.h"

/**
 * Implements a factory for UnrealPSKPSA skeletal mesh objects.
 */
 
UCLASS(hidecategories=Object)
class UNREALPSKPSA_API UPSKFactory : public UFactory
{
	GENERATED_UCLASS_BODY()
	
	virtual UObject* FactoryCreateFile(UClass* Class, UObject* Parent, FName Name, EObjectFlags Flags, const FString& Filename, const TCHAR* Params, FFeedbackContext* Warn, bool& bOutOperationCanceled) override;
	static void ProcessSkeleton(const FSkeletalMeshImportData&    ImportData,
	                            const USkeleton*                  Skeleton,
	                            FReferenceSkeleton&               OutRefSkeleton,
	                            int32&                            OutSkeletalDepth);
};
