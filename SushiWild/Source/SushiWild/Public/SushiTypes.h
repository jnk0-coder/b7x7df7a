#pragma once

#include "CoreMinimal.h"
#include "SushiTypes.generated.h"

// 도감 등급 (기획서 3-2). 순서는 SushiBalance::ERarity 와 같다.
UENUM(BlueprintType)
enum class ESushiRarity : uint8
{
	Common,
	Rare,
	Hero,
	Legend,
	Meme
};

// 날씨 (기획서 3-4). 순서는 SushiBalance::EWeather 와 같다.
UENUM(BlueprintType)
enum class ESushiWeather : uint8
{
	Clear,
	Cloudy,
	Rain,
	Fog
};

// 어떻게 잡았나 = 어떤 맛이 되나 (기획서 4-1 재료 스트레스).
UENUM(BlueprintType)
enum class ESushiCatchStyle : uint8
{
	Stealth,
	Rhythm,
	Brawl,
	Chase
};

// 손님 취향 레이더 / 초밥 맛 [매운맛, 단맛, 모험심, 비주얼, 양] (기획서 5-8).
USTRUCT(BlueprintType)
struct FSushiFlavor
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite) float Spicy = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float Sweet = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float Adventure = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float Visual = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float Volume = 0.f;

	FSushiFlavor() = default;
	FSushiFlavor(float InSpicy, float InSweet, float InAdventure, float InVisual, float InVolume)
		: Spicy(InSpicy), Sweet(InSweet), Adventure(InAdventure), Visual(InVisual), Volume(InVolume)
	{
	}

	void ToArray(double Out[5]) const
	{
		Out[0] = Spicy;
		Out[1] = Sweet;
		Out[2] = Adventure;
		Out[3] = Visual;
		Out[4] = Volume;
	}
};

// 가방 속 재료 한 개.
USTRUCT(BlueprintType)
struct FSushiIngredient
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite) FName Id;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FText DisplayName;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) ESushiRarity Rarity = ESushiRarity::Common;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) ESushiCatchStyle CatchStyle = ESushiCatchStyle::Stealth;

	// 0~100. 잡는 방식 보정까지 들어간 값.
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float Quality = 70.f;

	// 현실 분 단위 반감기. 0 이하면 상하지 않는다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float HalfLifeMinutes = 8.f;

	// 손에 넣은 순간의 월드 시간(초).
	UPROPERTY(EditAnywhere, BlueprintReadWrite) double AcquiredWorldSeconds = 0.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite) FSushiFlavor Flavor;
};

// 화면 위쪽에 잠깐 떴다 사라지는 개그 자막.
USTRUCT()
struct FSushiToast
{
	GENERATED_BODY()

	UPROPERTY() FText Text;
	UPROPERTY() FLinearColor Color = FLinearColor::White;
	UPROPERTY() float TimeLeft = 3.f;
};
