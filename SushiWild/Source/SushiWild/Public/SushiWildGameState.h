#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "SushiBalance.h"
#include "SushiTypes.h"
#include "SushiWildGameState.generated.h"

class ADirectionalLight;
class AExponentialHeightFog;

// 하루 시계 · 마르코프 날씨 · 돈 · 별점 · 도감 천장 · 개그 자막을 들고 있는 곳.
UCLASS()
class SUSHIWILD_API ASushiWildGameState : public AGameState
{
	GENERATED_BODY()

public:
	ASushiWildGameState();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	// ── 시계 (기획서 3-1: 게임 1시간 = 현실 1분) ──
	UPROPERTY(EditAnywhere, Category = "Sushi|Clock") float RealSecondsPerGameMinute = 1.f;
	UPROPERTY(VisibleAnywhere, Category = "Sushi|Clock") float GameMinutes = 5.f * 60.f;
	UPROPERTY(VisibleAnywhere, Category = "Sushi|Clock") int32 Day = 1;

	int32 GetHour() const { return FMath::FloorToInt(GameMinutes / 60.f) % 24; }
	bool IsNight() const { const int32 H = GetHour(); return H >= 22 || H < 5; }
	bool IsDawn() const { const int32 H = GetHour(); return H >= 5 && H < 10; }
	FText GetClockText() const;

	// ── 날씨 (기획서 3-4) ──
	UPROPERTY(VisibleAnywhere, Category = "Sushi|Weather") ESushiWeather Weather = ESushiWeather::Clear;
	FText GetWeatherText() const;
	// 다음 23~03시 창에 비가 한 번이라도 올 확률 (개굴레오 예보).
	float GetTonightRainChance() const;

	// 날씨·시간이 잠입에 주는 영향 (기획서 3-4, 4-2).
	float GetVisionRangeMultiplier() const;
	float GetNoiseMultiplier() const;

	// ── 가게 ──
	UPROPERTY(VisibleAnywhere, Category = "Sushi|Shop") int32 Coins = 0;
	UPROPERTY(VisibleAnywhere, Category = "Sushi|Shop") float StarRating = 3.f;
	UPROPERTY(VisibleAnywhere, Category = "Sushi|Shop") int32 ServedCount = 0;

	void AddCoins(int32 Amount);
	void ApplyReview(int32 Stars);
	// 별점이 떨어지면 간판 글자가 하나씩 떨어진다 (기획서 7-1 #4).
	FText GetSignText() const;

	// ── 도감 등급 굴리기 (천장 포함) ──
	ESushiRarity RollRarity();
	FRandomStream& GetRng() { return Rng; }

	// ── 보이지 않는 할배의 손 (기획서 7-4) ──
	void RegisterMissionResult(FName Mission, bool bSuccess);
	float GetJudgeWindowScale(FName Mission) const;

	// ── 농장 현상수배 (기획서 7-1 #2) ──
	UPROPERTY(VisibleAnywhere, Category = "Sushi|Farm") int32 FarmAlarmCount = 0;

	// ── 개그 자막 ──
	void PushToast(const FText& Text, const FLinearColor& Color = FLinearColor::White, float Duration = 3.5f);
	const TArray<FSushiToast>& GetToasts() const { return Toasts; }

	// 월드 빌더가 만든 해/안개를 넘겨받아 시간·날씨에 맞춰 움직인다.
	void SetEnvironment(ADirectionalLight* InSun, AExponentialHeightFog* InFog);

private:
	void AdvanceWeather();
	void UpdateEnvironment();

	UPROPERTY() TObjectPtr<ADirectionalLight> Sun;
	UPROPERTY() TObjectPtr<AExponentialHeightFog> Fog;
	UPROPERTY() TArray<FSushiToast> Toasts;

	TMap<FName, int32> ConsecutiveFails;
	SushiBalance::FPityState Pity;
	FRandomStream Rng;
	int32 LastWeatherSlot = -1;
	bool bSignFlipped = false;
};
