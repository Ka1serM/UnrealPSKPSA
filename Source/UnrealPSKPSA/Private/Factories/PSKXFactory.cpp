// Fill out your copyright notice in the Description page of Project Settings.

#include "Factories/PSKXFactory.h"
#include "Readers/PSKReader.h"
#include "RawMesh.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Editor/UnrealEd/Classes/Factories/MaterialInstanceConstantFactoryNew.h"
#include "EditorAssetLibrary.h"
#include "AssetToolsModule.h"
#include "ComponentReregisterContext.h"
#include "IAssetTools.h"
#include "Utils/ActorXUtils.h"

/* UTextAssetFactory structors
 *****************************************************************************/

UPSKXFactory::UPSKXFactory( const FObjectInitializer& ObjectInitializer )
	: Super(ObjectInitializer)
{
	Formats.Add(TEXT("pskx;PSKX Static Mesh File"));
	SupportedClass = UStaticMesh::StaticClass();
	bCreateNew = false;
	bEditorImport = true;
}

/* UFactory overrides
 *****************************************************************************/

UObject* UPSKXFactory::FactoryCreateFile(UClass* Class, UObject* Parent, FName Name, EObjectFlags Flags, const FString& Filename, const TCHAR* Params, FFeedbackContext* Warn, bool& bOutOperationCanceled)
{
	auto Data = PSKReader(Filename);
	if (!Data.Read()) return nullptr;
	
	TArray<FColor> VertexColorsByPoint;
	VertexColorsByPoint.Init(FColor::Black, Data.VertexColors.Num());
	if (Data.bHasVertexColors)
	{
		for (auto i = 0; i < Data.Wedges.Num(); i++)
		{
			auto FixedColor = Data.VertexColors[i];
			Swap(FixedColor.R, FixedColor.B);
			VertexColorsByPoint[Data.Wedges[i].PointIndex] = FixedColor;
		}
	}
	
	auto RawMesh = FRawMesh();
	for (auto Vertex : Data.Vertices)
	{
		auto FixedVertex = Vertex;
		FixedVertex.Y = -FixedVertex.Y; // MIRROR_MESH
		RawMesh.VertexPositions.Add(FixedVertex);
	}

	auto WindingOrder = {2, 1, 0};
	for (const auto PskFace : Data.Faces)
	{
		RawMesh.FaceMaterialIndices.Add(PskFace.MatIndex);
		RawMesh.FaceSmoothingMasks.Add(1);

		for (auto VertexIndex : WindingOrder)
		{
			const auto WedgeIndex = PskFace.WedgeIndex[VertexIndex];
			const auto PskWedge = Data.Wedges[WedgeIndex];

			RawMesh.WedgeIndices.Add(PskWedge.PointIndex);
			RawMesh.WedgeColors.Add(Data.bHasVertexColors ? VertexColorsByPoint[PskWedge.PointIndex] : FColor::Black);
			RawMesh.WedgeTexCoords[0].Add(FVector2f(PskWedge.U, PskWedge.V));
			for (auto UVIndex = 0; UVIndex < Data.ExtraUVs.Num(); UVIndex++)
			{
				auto UV =  Data.ExtraUVs[UVIndex][PskFace.WedgeIndex[VertexIndex]];
				RawMesh.WedgeTexCoords[UVIndex+1].Add(UV);
			}
			
			RawMesh.WedgeTangentZ.Add(Data.bHasVertexNormals ? Data.Normals[PskWedge.PointIndex] * FVector3f(1, -1, 1) : FVector3f::ZeroVector);
			RawMesh.WedgeTangentY.Add(FVector3f::ZeroVector);
			RawMesh.WedgeTangentX.Add(FVector3f::ZeroVector);
		}
	}

	const auto StaticMesh = CastChecked<UStaticMesh>(CreateOrOverwriteAsset(UStaticMesh::StaticClass(), Parent, Name, Flags));
	
	for (auto i = 0; i < Data.Materials.Num(); i++)
	{
		auto PskMaterial = Data.Materials[i];
		auto MaterialAdd = FActorXUtils::LocalFindOrCreate<UMaterial>(UMaterial::StaticClass(), Parent, PskMaterial.MaterialName, Flags);
		StaticMesh->GetStaticMaterials().Add(FStaticMaterial(MaterialAdd));
		StaticMesh->GetSectionInfoMap().Set(0, i, FMeshSectionInfo(i));
	}
	
	auto& SourceModel = StaticMesh->AddSourceModel();
	SourceModel.BuildSettings.bBuildReversedIndexBuffer = false;
	SourceModel.BuildSettings.bRecomputeTangents = false;
	SourceModel.BuildSettings.bGenerateLightmapUVs = false;
	SourceModel.BuildSettings.bComputeWeightedNormals = false;
	SourceModel.BuildSettings.bRecomputeNormals = !Data.bHasVertexNormals;
	SourceModel.SaveRawMesh(RawMesh);

	StaticMesh->Build();
	StaticMesh->PostEditChange();
	FAssetRegistryModule::AssetCreated(StaticMesh);
	StaticMesh->MarkPackageDirty();

	FGlobalComponentReregisterContext RecreateComponents;
	
	return StaticMesh;
}


