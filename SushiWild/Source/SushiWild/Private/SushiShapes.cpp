#include "SushiShapes.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

namespace
{
	const TCHAR* ShapePath(ESushiShape Shape)
	{
		switch (Shape)
		{
		case ESushiShape::Sphere:   return TEXT("/Engine/BasicShapes/Sphere.Sphere");
		case ESushiShape::Cylinder: return TEXT("/Engine/BasicShapes/Cylinder.Cylinder");
		case ESushiShape::Cone:     return TEXT("/Engine/BasicShapes/Cone.Cone");
		default:                    return TEXT("/Engine/BasicShapes/Cube.Cube");
		}
	}

	UMaterialInterface* BaseMaterial()
	{
		// 엔진 기본 도형 머티리얼. "Color" 벡터 파라미터를 가지고 있다.
		return LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	}
}

UStaticMeshComponent* SushiShapes::Add(AActor* Owner, USceneComponent* Parent, ESushiShape Shape,
	const FVector& RelativeLocation, const FVector& Scale, const FLinearColor& Color,
	bool bCollision, const FRotator& RelativeRotation)
{
	if (!Owner)
	{
		return nullptr;
	}

	UStaticMeshComponent* Mesh = NewObject<UStaticMeshComponent>(Owner);
	Mesh->SetMobility(EComponentMobility::Movable);
	Mesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, ShapePath(Shape)));
	if (bCollision)
	{
		Mesh->SetCollisionProfileName(TEXT("BlockAll"));
	}
	else
	{
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Mesh->SetGenerateOverlapEvents(false);
	}

	USceneComponent* AttachTo = Parent ? Parent : Owner->GetRootComponent();
	if (AttachTo)
	{
		Mesh->SetupAttachment(AttachTo);
	}
	Mesh->SetRelativeLocation(RelativeLocation);
	Mesh->SetRelativeRotation(RelativeRotation);
	Mesh->SetRelativeScale3D(Scale);
	Mesh->RegisterComponent();
	Owner->AddInstanceComponent(Mesh);

	if (!AttachTo)
	{
		Owner->SetRootComponent(Mesh);
	}

	SetColor(Mesh, Color);
	return Mesh;
}

void SushiShapes::SetColor(UStaticMeshComponent* Component, const FLinearColor& Color)
{
	if (!Component)
	{
		return;
	}
	UMaterialInstanceDynamic* Mid = Cast<UMaterialInstanceDynamic>(Component->GetMaterial(0));
	if (!Mid)
	{
		Mid = Component->CreateDynamicMaterialInstance(0, BaseMaterial());
	}
	if (Mid)
	{
		Mid->SetVectorParameterValue(TEXT("Color"), Color);
	}
}

UMaterialInstanceDynamic* SushiShapes::MakeColorMaterial(UObject* Outer, const FLinearColor& Color)
{
	UMaterialInstanceDynamic* Mid = UMaterialInstanceDynamic::Create(BaseMaterial(), Outer);
	if (Mid)
	{
		Mid->SetVectorParameterValue(TEXT("Color"), Color);
	}
	return Mid;
}
