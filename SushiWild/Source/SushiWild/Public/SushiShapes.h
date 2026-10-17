// 엔진 기본 도형(큐브·구·원기둥·원뿔)으로 회색 박스 비주얼을 조립하는 도우미.
// 아트 에셋이 들어오기 전까지 모든 캐릭터·소품은 이걸로 만든다.
#pragma once

#include "CoreMinimal.h"

class AActor;
class USceneComponent;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;

enum class ESushiShape : uint8
{
	Cube,
	Sphere,
	Cylinder,
	Cone
};

namespace SushiShapes
{
	// Scale 1 = 100cm 도형. Collision 이 true 면 BlockAll.
	UStaticMeshComponent* Add(AActor* Owner, USceneComponent* Parent, ESushiShape Shape,
		const FVector& RelativeLocation, const FVector& Scale, const FLinearColor& Color,
		bool bCollision = false, const FRotator& RelativeRotation = FRotator::ZeroRotator);

	// 이미 붙어 있는 도형의 색만 바꾼다.
	void SetColor(UStaticMeshComponent* Component, const FLinearColor& Color);

	// 바닥 부채꼴(시야 원뿔) 등에 쓰는 단색 머티리얼.
	UMaterialInstanceDynamic* MakeColorMaterial(UObject* Outer, const FLinearColor& Color);
}
