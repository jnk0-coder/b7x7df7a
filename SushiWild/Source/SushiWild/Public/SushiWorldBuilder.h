#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SushiWorldBuilder.generated.h"

class ASushiWildGameState;

// 맵 에셋 없이 섬 전체(마을·농장·해변·숲·하늘·해)를 C++로 세운다.
// 레벨 아티스트가 진짜 맵을 만들기 전까지 쓰는 회색 박스 월드 (기획서 2-1).
UCLASS()
class SUSHIWILD_API ASushiWorldBuilder : public AActor
{
	GENERATED_BODY()

public:
	ASushiWorldBuilder();

	// 게임 모드가 경기 시작 전에 한 번 부른다. 플레이어 시작 위치를 돌려준다.
	FTransform Build(ASushiWildGameState* State);

	UPROPERTY(EditAnywhere, Category = "Sushi|World") int32 Seed = 20261017;

private:
	void BuildSky(ASushiWildGameState* State);
	void BuildGround();
	void BuildVillage();
	void BuildFarm();
	void BuildBeach();
	void BuildForest();

	// 땅에 놓이는 상자 (Size 는 cm, 바닥 기준)
	void AddBlock(const FVector& FootCenter, const FVector& SizeCm, const FLinearColor& Color, bool bCollision = true, float Yaw = 0.f);
	void AddPatch(const FVector& Center, float SizeX, float SizeY, const FLinearColor& Color);
	void AddTree(const FVector& Foot, float Height);
	void AddCover(const FVector& FootCenter, const FVector& SizeCm, const FLinearColor& Color);

	FRandomStream Rng;
};
