#include "SushiWildGameState.h"

#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/ExponentialHeightFog.h"

#define LOCTEXT_NAMESPACE "SushiGameState"

namespace
{
	constexpr float MinutesPerWeatherSlot = 240.f; // 게임 속 4시간마다 날씨가 바뀐다
	constexpr int32 SlotsPerDay = 6;
	constexpr int32 NightWindowSlot = 5;           // 20~24시 칸 (+ 다음 날 00~04시 칸)
}

ASushiWildGameState::ASushiWildGameState()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
}

void ASushiWildGameState::BeginPlay()
{
	Super::BeginPlay();
	Rng.GenerateNewSeed();
	LastWeatherSlot = FMath::FloorToInt(GameMinutes / MinutesPerWeatherSlot);
	PushToast(LOCTEXT("Intro", "할배: 시장은 스시마트가 다 사버렸슈. 재료는 직접 구해 와유!"), FLinearColor(1.f, 0.9f, 0.6f), 6.f);
	PushToast(FText::Format(LOCTEXT("FirstForecast", "개굴레오: 오늘 밤 비 올 확률 {0}%… 근데 제 적중률은 70%예요. 개굴."),
		FText::AsNumber(FMath::RoundToInt(GetTonightRainChance() * 100.f))), FLinearColor(0.6f, 1.f, 0.6f), 6.f);
}

void ASushiWildGameState::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	GameMinutes += DeltaSeconds / FMath::Max(RealSecondsPerGameMinute, 0.01f);
	if (GameMinutes >= 1440.f)
	{
		GameMinutes -= 1440.f;
		++Day;
		LastWeatherSlot -= SlotsPerDay;
		PushToast(FText::Format(LOCTEXT("NewDay", "{0}일째 새벽. 개굴레오: 오늘 밤 비 {1}%"),
			FText::AsNumber(Day), FText::AsNumber(FMath::RoundToInt(GetTonightRainChance() * 100.f))), FLinearColor(0.6f, 1.f, 0.6f));
	}

	const int32 Slot = FMath::FloorToInt(GameMinutes / MinutesPerWeatherSlot);
	if (Slot != LastWeatherSlot)
	{
		LastWeatherSlot = Slot;
		AdvanceWeather();
	}

	for (int32 i = Toasts.Num() - 1; i >= 0; --i)
	{
		Toasts[i].TimeLeft -= DeltaSeconds;
		if (Toasts[i].TimeLeft <= 0.f)
		{
			Toasts.RemoveAt(i);
		}
	}

	UpdateEnvironment();
}

FText ASushiWildGameState::GetClockText() const
{
	const int32 Total = FMath::FloorToInt(GameMinutes);
	return FText::Format(LOCTEXT("Clock", "{0}일째  {1}:{2}"),
		FText::AsNumber(Day),
		FText::FromString(FString::Printf(TEXT("%02d"), (Total / 60) % 24)),
		FText::FromString(FString::Printf(TEXT("%02d"), Total % 60)));
}

FText ASushiWildGameState::GetWeatherText() const
{
	switch (Weather)
	{
	case ESushiWeather::Cloudy: return LOCTEXT("Cloudy", "흐림");
	case ESushiWeather::Rain:   return LOCTEXT("Rain", "비 (발소리가 묻힘)");
	case ESushiWeather::Fog:    return LOCTEXT("Fog", "안개 (경비 시야 절반)");
	default:                    return LOCTEXT("Clear", "맑음");
	}
}

float ASushiWildGameState::GetTonightRainChance() const
{
	const int32 SlotToday = FMath::FloorToInt(GameMinutes / MinutesPerWeatherSlot) % SlotsPerDay;
	const int32 Steps = (NightWindowSlot - SlotToday + SlotsPerDay) % SlotsPerDay;
	return static_cast<float>(SushiBalance::RainChanceInNightWindow(static_cast<SushiBalance::EWeather>(Weather), Steps));
}

float ASushiWildGameState::GetVisionRangeMultiplier() const
{
	float Mult = 1.f;
	if (Weather == ESushiWeather::Fog)
	{
		Mult *= 0.5f;
	}
	else if (Weather == ESushiWeather::Rain)
	{
		Mult *= 0.85f;
	}
	if (IsDawn())
	{
		Mult *= 0.7f; // 새벽엔 경비들이 졸려서 은신이 쉽다
	}
	return Mult;
}

float ASushiWildGameState::GetNoiseMultiplier() const
{
	return Weather == ESushiWeather::Rain ? 0.6f : 1.f;
}

void ASushiWildGameState::AddCoins(int32 Amount)
{
	Coins = FMath::Max(0, Coins + Amount);
}

void ASushiWildGameState::ApplyReview(int32 Stars)
{
	const bool bWasLow = StarRating < 1.5f;
	const FText OldSign = GetSignText();

	StarRating = FMath::Clamp(FMath::Lerp(StarRating, static_cast<float>(Stars), 0.25f), 0.f, 5.f);
	++ServedCount;

	// 별점이 회복되면 글자가 다시 붙는데, 가끔 거꾸로 붙는다.
	bSignFlipped = bWasLow && StarRating >= 2.5f && Rng.FRand() < 0.25f;

	const FText NewSign = GetSignText();
	if (!NewSign.EqualTo(OldSign))
	{
		PushToast(FText::Format(LOCTEXT("SignChanged", "간판이 바뀌었슈: \"{0}\" → \"{1}\""), OldSign, NewSign), FLinearColor(1.f, 0.6f, 0.3f), 5.f);
	}
}

FText ASushiWildGameState::GetSignText() const
{
	if (StarRating >= 2.5f)
	{
		return bSignFlipped ? LOCTEXT("SignFlipped", "왕밥초") : LOCTEXT("SignFull", "초밥왕");
	}
	if (StarRating >= 1.5f) return LOCTEXT("SignSushi", "초밥");
	if (StarRating >= 0.75f) return LOCTEXT("SignRice", "밥");
	return LOCTEXT("SignB", "ㅂ");
}

ESushiRarity ASushiWildGameState::RollRarity()
{
	return static_cast<ESushiRarity>(SushiBalance::RollRarity(Rng.FRand(), Pity));
}

void ASushiWildGameState::RegisterMissionResult(FName Mission, bool bSuccess)
{
	int32& Fails = ConsecutiveFails.FindOrAdd(Mission);
	if (bSuccess)
	{
		Fails = 0;
		return;
	}
	++Fails;
	if (Fails == 3 || Fails == 5)
	{
		PushToast(LOCTEXT("GrandpaHint", "할배: 급할수록 천천히 혀유~ 밥알은 도망 안 가유. …참치는 도망가유."), FLinearColor(1.f, 0.9f, 0.6f), 5.f);
	}
}

float ASushiWildGameState::GetJudgeWindowScale(FName Mission) const
{
	const int32* Fails = ConsecutiveFails.Find(Mission);
	return 1.f + static_cast<float>(SushiBalance::DdaWindowBonus(Fails ? *Fails : 0));
}

void ASushiWildGameState::PushToast(const FText& Text, const FLinearColor& Color, float Duration)
{
	FSushiToast& Toast = Toasts.AddDefaulted_GetRef();
	Toast.Text = Text;
	Toast.Color = Color;
	Toast.TimeLeft = Duration;
	while (Toasts.Num() > 6)
	{
		Toasts.RemoveAt(0);
	}
}

void ASushiWildGameState::SetEnvironment(ADirectionalLight* InSun, AExponentialHeightFog* InFog)
{
	Sun = InSun;
	Fog = InFog;
	UpdateEnvironment();
}

void ASushiWildGameState::AdvanceWeather()
{
	const ESushiWeather Old = Weather;
	Weather = static_cast<ESushiWeather>(SushiBalance::NextWeather(static_cast<SushiBalance::EWeather>(Weather), Rng.FRand()));
	if (Weather != Old)
	{
		PushToast(FText::Format(LOCTEXT("WeatherChanged", "날씨가 바뀌었슈: {0}"), GetWeatherText()), FLinearColor(0.6f, 0.85f, 1.f));
	}
}

void ASushiWildGameState::UpdateEnvironment()
{
	if (Sun)
	{
		// 06시 해돋이(0°) → 12시 머리 위(-90°) → 18시 해넘이. 밤엔 해가 땅 밑으로.
		const float Pitch = -((GameMinutes - 360.f) / 1440.f) * 360.f;
		Sun->SetActorRotation(FRotator(Pitch, -40.f, 0.f));

		float Lux = 10.f;
		switch (Weather)
		{
		case ESushiWeather::Cloudy: Lux = 6.f; break;
		case ESushiWeather::Rain:   Lux = 3.f; break;
		case ESushiWeather::Fog:    Lux = 5.f; break;
		default: break;
		}
		if (ULightComponent* Light = Sun->GetLightComponent())
		{
			Light->SetIntensity(Lux);
		}
	}

	if (Fog)
	{
		float Density = 0.01f;
		switch (Weather)
		{
		case ESushiWeather::Cloudy: Density = 0.02f; break;
		case ESushiWeather::Rain:   Density = 0.05f; break;
		case ESushiWeather::Fog:    Density = 0.3f; break;
		default: break;
		}
		if (UExponentialHeightFogComponent* FogComp = Fog->GetComponent())
		{
			FogComp->SetFogDensity(Density);
		}
	}
}

#undef LOCTEXT_NAMESPACE
