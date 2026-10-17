#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "SushiVisionComponent.generated.h"

class ASushiChefCharacter;
class UMaterialInstanceDynamic;
class UProceduralMeshComponent;

// 경비 동물의 눈과 귀. 바닥에 부채꼴(시야 원뿔)을 그리고,
// 노랑 = 평온, 주황 = 의심, 빨강 = 들킴 으로 색이 바뀐다 (기획서 4-1).
UCLASS(ClassGroup = (Sushi), meta = (BlueprintSpawnableComponent))
class SUSHIWILD_API USushiVisionComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	USushiVisionComponent();

	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, Category = "Sushi|Vision") float Range = 1500.f;
	UPROPERTY(EditAnywhere, Category = "Sushi|Vision") float HalfAngleDeg = 35.f;
	UPROPERTY(EditAnywhere, Category = "Sushi|Vision") float HearingRange = 900.f;
	UPROPERTY(EditAnywhere, Category = "Sushi|Vision") int32 ConeSegments = 16;

	// 0 이면 안 보임. 가까울수록, 서 있을수록 크다 (최대 약 1.2).
	float ComputeSight(const ASushiChefCharacter* Chef, float RangeMultiplier) const;

	// 0 이면 안 들림. 발소리 × 거리 감쇠.
	float ComputeHearing(const ASushiChefCharacter* Chef, float NoiseMultiplier) const;

	// 0 = 평온(노랑) … 1 = 들킴(빨강)
	void SetAlertLevel(float Alert01);

	// 날씨 때문에 시야가 줄면 부채꼴도 같이 줄인다.
	void SetRangeMultiplier(float Multiplier);

private:
	UPROPERTY() TObjectPtr<UProceduralMeshComponent> Fan;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> FanMaterial;
	float LastAlert = -1.f;
};
