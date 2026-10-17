#include "SushiWorldBuilder.h"

#include "ChickGuard.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/SkyLight.h"
#include "Engine/World.h"
#include "FishingSpot.h"
#include "KokoleoneNest.h"
#include "SushiCounter.h"
#include "SushiShapes.h"
#include "SushiWildGameState.h"
#include "SushiWorldLayout.h"

namespace
{
	const FLinearColor Grass(0.32f, 0.62f, 0.25f);
	const FLinearColor Sand(0.93f, 0.85f, 0.6f);
	const FLinearColor Dirt(0.55f, 0.45f, 0.3f);
	const FLinearColor Wood(0.5f, 0.33f, 0.18f);
	const FLinearColor Hay(0.95f, 0.8f, 0.3f);
	const FLinearColor Box(0.72f, 0.55f, 0.35f);

	FVector Polar(const FVector& Center, float Radius, float Degrees)
	{
		const float R = FMath::DegreesToRadians(Degrees);
		return Center + FVector(FMath::Cos(R), FMath::Sin(R), 0.f) * Radius;
	}

	template <typename T>
	T* SpawnAt(UWorld* World, const FVector& Location, const FRotator& Rotation = FRotator::ZeroRotator)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		return World->SpawnActor<T>(T::StaticClass(), Location, Rotation, Params);
	}
}

ASushiWorldBuilder::ASushiWorldBuilder()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

FTransform ASushiWorldBuilder::Build(ASushiWildGameState* State)
{
	Rng.Initialize(Seed);
	BuildSky(State);
	BuildGround();
	BuildVillage();
	BuildFarm();
	BuildBeach();
	BuildForest();

	// 포장마차 카운터 뒤(요리사 자리)에서 해변 쪽(+X)을 보고 시작
	return FTransform(FRotator::ZeroRotator, FVector(0.f, 0.f, 110.f));
}

void ASushiWorldBuilder::AddBlock(const FVector& FootCenter, const FVector& SizeCm, const FLinearColor& Color, bool bCollision, float Yaw)
{
	SushiShapes::Add(this, nullptr, ESushiShape::Cube, FootCenter + FVector(0.f, 0.f, SizeCm.Z * 0.5f), SizeCm / 100.f, Color, bCollision, FRotator(0.f, Yaw, 0.f));
}

void ASushiWorldBuilder::AddPatch(const FVector& Center, float SizeX, float SizeY, const FLinearColor& Color)
{
	AddBlock(FVector(Center.X, Center.Y, 0.f), FVector(SizeX, SizeY, 1.f), Color, false);
}

void ASushiWorldBuilder::AddTree(const FVector& Foot, float Height)
{
	SushiShapes::Add(this, nullptr, ESushiShape::Cylinder, Foot + FVector(0.f, 0.f, Height * 0.5f), FVector(0.4f, 0.4f, Height / 100.f), Wood, true);
	SushiShapes::Add(this, nullptr, ESushiShape::Cone, Foot + FVector(0.f, 0.f, Height + 60.f), FVector(2.6f, 2.6f, 2.4f), FLinearColor(0.15f, 0.45f, 0.2f));
}

void ASushiWorldBuilder::AddCover(const FVector& FootCenter, const FVector& SizeCm, const FLinearColor& Color)
{
	// 건초더미·골판지 상자: 웅크리면 대부·병아리 시선이 막힌다
	const FVector ToCenter = SushiLayout::Farm - FootCenter;
	const float Yaw = FMath::RadiansToDegrees(FMath::Atan2(ToCenter.Y, ToCenter.X)) + 90.f;
	AddBlock(FootCenter, SizeCm, Color, true, Yaw);
}

void ASushiWorldBuilder::BuildSky(ASushiWildGameState* State)
{
	UWorld* World = GetWorld();

	ADirectionalLight* Sun = SpawnAt<ADirectionalLight>(World, FVector(0.f, 0.f, 2000.f), FRotator(-45.f, -40.f, 0.f));
	if (Sun)
	{
		Sun->SetMobility(EComponentMobility::Movable);
		if (UDirectionalLightComponent* SunLight = Cast<UDirectionalLightComponent>(Sun->GetLightComponent()))
		{
			SunLight->SetAtmosphereSunLight(true);
			SunLight->SetIntensity(10.f);
		}
	}

	SpawnAt<ASkyAtmosphere>(World, FVector::ZeroVector);

	if (ASkyLight* Sky = SpawnAt<ASkyLight>(World, FVector(0.f, 0.f, 1000.f)))
	{
		if (USkyLightComponent* SkyLight = Sky->GetLightComponent())
		{
			SkyLight->SetMobility(EComponentMobility::Movable);
			SkyLight->bRealTimeCapture = true;
			SkyLight->SourceType = ESkyLightSourceType::SLS_CapturedScene;
			SkyLight->MarkRenderStateDirty();
			SkyLight->RecaptureSky();
		}
	}

	AExponentialHeightFog* Fog = SpawnAt<AExponentialHeightFog>(World, FVector::ZeroVector);

	if (State)
	{
		State->SetEnvironment(Sun, Fog);
	}
}

void ASushiWorldBuilder::BuildGround()
{
	const float Size = SushiLayout::WorldHalfSize * 2.f;
	AddBlock(FVector(0.f, 0.f, -60.f), FVector(Size, Size, 60.f), Grass, true);

	// 마을에서 각 지역으로 가는 흙길
	AddPatch(FVector(-4250.f, 0.f, 0.f), 4500.f, 400.f, Dirt);
	AddPatch(FVector(4500.f, 0.f, 0.f), 5000.f, 400.f, Dirt);
	AddPatch(FVector(0.f, 4500.f, 0.f), 400.f, 5000.f, Dirt);
}

void ASushiWorldBuilder::BuildVillage()
{
	UWorld* World = GetWorld();
	AddPatch(SushiLayout::Village, 4000.f, 4000.f, FLinearColor(0.85f, 0.78f, 0.6f));

	SpawnAt<ASushiCounter>(World, SushiLayout::Counter);

	// 와사비 할배 집 + 우물 + 이정표
	AddBlock(FVector(-700.f, 1000.f, 0.f), FVector(600.f, 500.f, 320.f), FLinearColor(0.9f, 0.85f, 0.75f));
	SushiShapes::Add(this, nullptr, ESushiShape::Cone, FVector(-700.f, 1000.f, 420.f), FVector(7.5f, 6.5f, 2.f), FLinearColor(0.35f, 0.25f, 0.2f));
	SushiShapes::Add(this, nullptr, ESushiShape::Cylinder, FVector(-400.f, -900.f, 50.f), FVector(1.2f, 1.2f, 1.f), FLinearColor(0.55f, 0.55f, 0.6f), true);
	AddBlock(FVector(-1600.f, 250.f, 0.f), FVector(20.f, 20.f, 220.f), Wood);
	AddBlock(FVector(-1600.f, 250.f, 180.f), FVector(10.f, 160.f, 50.f), FLinearColor(0.95f, 0.9f, 0.7f), false);
	AddBlock(FVector(1600.f, 250.f, 0.f), FVector(20.f, 20.f, 220.f), Wood);
	AddBlock(FVector(1600.f, 250.f, 180.f), FVector(10.f, 160.f, 50.f), FLinearColor(0.7f, 0.85f, 1.f), false);
}

void ASushiWorldBuilder::BuildFarm()
{
	UWorld* World = GetWorld();
	const FVector Farm = SushiLayout::Farm;
	const float Half = 2250.f;
	AddPatch(Farm, Half * 2.f, Half * 2.f, FLinearColor(0.5f, 0.55f, 0.3f));

	// 울타리: 동·북·남쪽에 틈 (옆으로 돌아 들어가 대부 등 뒤를 노릴 것)
	const FLinearColor Fence(0.75f, 0.6f, 0.4f);
	const float Gap = 400.f;
	const float SideLen = Half - Gap;
	for (const float Sign : { -1.f, 1.f })
	{
		// 동쪽 (마을 쪽) 벽 두 토막
		AddBlock(Farm + FVector(Half, Sign * (Gap + SideLen * 0.5f), 0.f), FVector(30.f, SideLen, 110.f), Fence);
		// 서쪽 벽 (틈 없음)
		AddBlock(Farm + FVector(-Half, Sign * Half * 0.5f, 0.f), FVector(30.f, Half, 110.f), Fence);
		// 북·남쪽 벽: 서쪽 절반에 틈
		AddBlock(Farm + FVector(Half * 0.5f, Sign * Half, 0.f), FVector(Half, 30.f, 110.f), Fence);
		AddBlock(Farm + FVector(-Half + (Half - Gap) * 0.5f - Gap * 0.5f, Sign * Half, 0.f), FVector(Half - Gap * 2.f, 30.f, 110.f), Fence);
	}

	// 빨간 헛간
	AddBlock(Farm + FVector(-1400.f, -1400.f, 0.f), FVector(400.f, 300.f, 300.f), FLinearColor(0.7f, 0.15f, 0.1f), true, 45.f);

	// 건초더미·골판지 상자 (병아리 순찰 원 사이에 배치)
	const struct { float Radius; float Degrees; bool bHay; } Covers[] = {
		{ 780.f, 30.f, true }, { 780.f, 150.f, false }, { 780.f, 250.f, true },
		{ 1130.f, 90.f, false }, { 1130.f, 200.f, true }, { 1130.f, 320.f, true },
		{ 1500.f, 10.f, true }, { 1500.f, 130.f, false }, { 1500.f, 230.f, true }, { 1700.f, 300.f, false },
	};
	for (const auto& Cover : Covers)
	{
		const FVector Foot = Polar(Farm, Cover.Radius, Cover.Degrees);
		if (Cover.bHay)
		{
			AddCover(Foot, FVector(200.f, 100.f, 110.f), Hay);
		}
		else
		{
			AddCover(Foot, FVector(100.f, 100.f, 100.f), Box);
		}
	}

	// 돈 꼬꼬레오네 (동쪽 = 농장 입구를 보고 앉아 있다) + 삐약 형제 3마리
	AKokoleoneNest* Nest = SpawnAt<AKokoleoneNest>(World, Farm);
	const struct { float Radius; float Speed; float Start; } Patrols[] = {
		{ 600.f, 0.35f, 0.f }, { 950.f, -0.25f, 2.f }, { 1300.f, 0.18f, 4.f },
	};
	for (const auto& Patrol : Patrols)
	{
		if (AChickGuard* Chick = SpawnAt<AChickGuard>(World, Polar(Farm, Patrol.Radius, FMath::RadiansToDegrees(Patrol.Start)) + FVector(0.f, 0.f, 36.f)))
		{
			Chick->SetupPatrol(Nest, Farm, Patrol.Radius, Patrol.Speed, Patrol.Start);
		}
	}
}

void ASushiWorldBuilder::BuildBeach()
{
	UWorld* World = GetWorld();
	const float ShoreX = SushiLayout::ShoreX;

	AddPatch(FVector((7000.f + ShoreX) * 0.5f, 0.f, 0.f), ShoreX - 7000.f, 8000.f, Sand);
	// 바다 (부딪힘 없음 — 들어가면 갈매기가 건져 준다)
	const float SeaFar = SushiLayout::WorldHalfSize - 500.f;
	AddBlock(FVector((ShoreX + SeaFar) * 0.5f, 0.f, 0.f), FVector(SeaFar - ShoreX, SushiLayout::WorldHalfSize * 2.f - 1000.f, 3.f), FLinearColor(0.1f, 0.4f, 0.75f), false);

	// 부두 (높이 40cm → 계단 없이 올라설 수 있다)
	const float PierLen = SushiLayout::PierEndX - ShoreX;
	AddBlock(FVector(ShoreX + PierLen * 0.5f, 0.f, 0.f), FVector(PierLen, SushiLayout::PierHalfWidth * 2.f, SushiLayout::PierDeckTop), Wood);
	for (float X = ShoreX + 200.f; X < SushiLayout::PierEndX; X += 500.f)
	{
		for (const float Sign : { -1.f, 1.f })
		{
			AddBlock(FVector(X, Sign * (SushiLayout::PierHalfWidth + 15.f), 0.f), FVector(25.f, 25.f, 110.f), Wood, false);
		}
	}
	SpawnAt<AFishingSpot>(World, SushiLayout::FishingSpot);

	// 파라솔 + 갈매기 날강도단(장식)
	const FLinearColor UmbrellaColors[] = { FLinearColor(1.f, 0.3f, 0.3f), FLinearColor(0.3f, 0.6f, 1.f), FLinearColor(1.f, 0.85f, 0.2f) };
	for (int32 i = 0; i < 3; ++i)
	{
		const FVector Foot(8200.f + i * 600.f, -1500.f + i * 1200.f, 0.f);
		SushiShapes::Add(this, nullptr, ESushiShape::Cylinder, Foot + FVector(0.f, 0.f, 120.f), FVector(0.06f, 0.06f, 2.4f), FLinearColor::White);
		SushiShapes::Add(this, nullptr, ESushiShape::Cone, Foot + FVector(0.f, 0.f, 260.f), FVector(2.4f, 2.4f, 0.6f), UmbrellaColors[i]);
	}
	for (int32 i = 0; i < 4; ++i)
	{
		const FVector Gull(ShoreX + 300.f + i * 500.f, SushiLayout::PierHalfWidth + 15.f, 125.f);
		SushiShapes::Add(this, nullptr, ESushiShape::Sphere, Gull, FVector(0.35f, 0.25f, 0.25f), FLinearColor::White);
		SushiShapes::Add(this, nullptr, ESushiShape::Cone, Gull + FVector(22.f, 0.f, 4.f), FVector(0.08f), FLinearColor(1.f, 0.7f, 0.1f), false, FRotator(-90.f, 0.f, 0.f));
	}
}

void ASushiWorldBuilder::BuildForest()
{
	const FVector Forest = SushiLayout::Forest;
	AddPatch(Forest, 7000.f, 5000.f, FLinearColor(0.2f, 0.42f, 0.18f));
	for (int32 i = 0; i < 45; ++i)
	{
		const FVector Foot = Forest + FVector(Rng.FRandRange(-3300.f, 3300.f), Rng.FRandRange(-2300.f, 2300.f), 0.f);
		if (FVector::Dist2D(Foot, FVector(Forest.X, Forest.Y - 2300.f, 0.f)) < 500.f)
		{
			continue; // 숲 입구는 비워 둔다
		}
		AddTree(Foot, Rng.FRandRange(300.f, 600.f));
	}
	// '공사 중' 표지판 (멧돼지 추격전은 v0.2)
	AddBlock(Forest + FVector(300.f, -2600.f, 0.f), FVector(20.f, 20.f, 200.f), Wood);
	AddBlock(Forest + FVector(300.f, -2600.f, 160.f), FVector(10.f, 200.f, 80.f), FLinearColor(1.f, 0.8f, 0.1f), false);
}
