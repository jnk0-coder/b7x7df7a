// SushiBalance.h
// 게임 밸런스 수식 모음 (기획서 3·4·5·6·7장의 숫자를 그대로 코드로 옮김).
// 엔진 타입을 쓰지 않는 순수 C++이라 Unreal 없이도 Tests/BalanceTest.cpp 로 검증할 수 있다.
#pragma once

#include <cmath>

namespace SushiBalance
{
	// ── 신선도 (기획서 4-1) ─────────────────────────────────────────
	// 신선도(t) = 100 × (1/2)^(t / 반감기). 반감기 <= 0 이면 상하지 않는 재료(쌀 등).
	inline double Freshness(double ElapsedMinutes, double HalfLifeMinutes)
	{
		if (HalfLifeMinutes <= 0.0 || ElapsedMinutes <= 0.0)
		{
			return 100.0;
		}
		return 100.0 * std::pow(0.5, ElapsedMinutes / HalfLifeMinutes);
	}

	// 신선도 보정 = 0.5 + 0.5 × (신선도 / 100). 신선도 0%면 점수 반토막.
	inline double FreshnessMultiplier(double FreshnessPercent)
	{
		const double F = FreshnessPercent < 0.0 ? 0.0 : (FreshnessPercent > 100.0 ? 100.0 : FreshnessPercent);
		return 0.5 + 0.5 * (F / 100.0);
	}

	// ── 도감 등급 확률 + 천장 (기획서 3-2) ───────────────────────────
	enum ERarity { Common = 0, Rare = 1, Hero = 2, Legend = 3, Meme = 4 };

	constexpr double RarityChance[5] = { 0.60, 0.28, 0.09, 0.025, 0.005 };
	constexpr int HeroPity = 30;    // 30번째 확정
	constexpr int LegendPity = 100; // 100번째 확정

	struct FPityState
	{
		int HeroMisses = 0;   // 영웅 이상이 안 나온 연속 횟수
		int LegendMisses = 0; // 전설이 안 나온 연속 횟수
	};

	// U 는 [0,1) 균등 난수. 밈급은 '단서'로만 나온다(실제 획득은 조건 퍼즐).
	inline ERarity RollRarity(double U, FPityState& Pity)
	{
		const bool bNaturalLegend = U >= RarityChance[Meme] && U < RarityChance[Meme] + RarityChance[Legend];
		ERarity Result;
		if (Pity.LegendMisses + 1 >= LegendPity || bNaturalLegend)
		{
			Result = Legend;
		}
		else if (Pity.HeroMisses + 1 >= HeroPity)
		{
			// 영웅 천장이 밈급 단서보다 우선한다 (단서는 다음 기회에).
			Result = Hero;
		}
		else if (U < RarityChance[Meme])
		{
			Result = Meme;
		}
		else if (U < RarityChance[Meme] + RarityChance[Legend] + RarityChance[Hero])
		{
			Result = Hero;
		}
		else if (U < RarityChance[Meme] + RarityChance[Legend] + RarityChance[Hero] + RarityChance[Rare])
		{
			Result = Rare;
		}
		else
		{
			Result = Common;
		}

		Pity.LegendMisses = (Result == Legend) ? 0 : Pity.LegendMisses + 1;
		Pity.HeroMisses = (Result == Hero || Result == Legend) ? 0 : Pity.HeroMisses + 1;
		return Result;
	}

	// n번 시도해서 한 번이라도 나올 확률 = 1 - (1-p)^n
	inline double ProbAtLeastOne(double P, int N)
	{
		return 1.0 - std::pow(1.0 - P, N);
	}

	// ── 날씨 마르코프 연쇄 (기획서 3-4) ─────────────────────────────
	enum EWeather { Clear = 0, Cloudy = 1, Rain = 2, Fog = 3 };

	// [지금][다음] 전이 확률. 게임 속 4시간마다 한 번 바뀐다.
	constexpr double WeatherTransition[4][4] = {
		{ 0.60, 0.25, 0.10, 0.05 },
		{ 0.30, 0.35, 0.25, 0.10 },
		{ 0.15, 0.35, 0.40, 0.10 },
		{ 0.40, 0.30, 0.10, 0.20 },
	};

	inline EWeather NextWeather(EWeather Current, double U)
	{
		double Acc = 0.0;
		for (int W = 0; W < 4; ++W)
		{
			Acc += WeatherTransition[Current][W];
			if (U < Acc)
			{
				return static_cast<EWeather>(W);
			}
		}
		return Fog;
	}

	// Steps 칸 뒤의 날씨 분포 (전이행렬 거듭제곱).
	inline void WeatherDistributionAfter(EWeather Current, int Steps, double Out[4])
	{
		double Dist[4] = { 0, 0, 0, 0 };
		Dist[Current] = 1.0;
		for (int S = 0; S < Steps; ++S)
		{
			double Next[4] = { 0, 0, 0, 0 };
			for (int From = 0; From < 4; ++From)
			{
				for (int To = 0; To < 4; ++To)
				{
					Next[To] += Dist[From] * WeatherTransition[From][To];
				}
			}
			for (int W = 0; W < 4; ++W)
			{
				Dist[W] = Next[W];
			}
		}
		for (int W = 0; W < 4; ++W)
		{
			Out[W] = Dist[W];
		}
	}

	// 오래 플레이했을 때 자리 잡는 날씨 비율 (정상분포).
	inline void StationaryWeather(double Out[4])
	{
		WeatherDistributionAfter(Clear, 500, Out);
	}

	// 전설 오징어 창(23~03시 = 날씨 칸 2개) 중 한 번이라도 비가 올 확률.
	// StepsToWindow: 지금 칸에서 창의 첫 칸(20~24시)까지 몇 칸 남았는지. 0이면 지금이 첫 칸.
	inline double RainChanceInNightWindow(EWeather Current, int StepsToWindow)
	{
		double First[4];
		WeatherDistributionAfter(Current, StepsToWindow, First);
		double NoRain = 0.0;
		for (int W = 0; W < 4; ++W)
		{
			if (W != Rain)
			{
				NoRain += First[W] * (1.0 - WeatherTransition[W][Rain]);
			}
		}
		return 1.0 - NoRain;
	}

	// ── 돌발 이벤트 포아송 스케줄러 (기획서 6-2) ─────────────────────
	inline double PoissonPmf(int K, double Lambda)
	{
		double Fact = 1.0;
		for (int I = 2; I <= K; ++I)
		{
			Fact *= I;
		}
		return std::exp(-Lambda) * std::pow(Lambda, K) / Fact;
	}

	// DeltaHours 동안 이벤트가 터질 확률 = 1 - e^(-λ·Δt)
	inline bool ShouldFireEvent(double LambdaPerHour, double DeltaHours, double U)
	{
		return U < 1.0 - std::exp(-LambdaPerHour * DeltaHours);
	}

	// ── 리듬 판정 (기획서 4-2 리듬 낚시, 5-3 샤리) ───────────────────
	enum EBeatJudge { Perfect = 0, Good = 1, Miss = 2 };

	constexpr double PerfectWindowSec = 0.07;
	constexpr double GoodWindowSec = 0.14;

	inline EBeatJudge JudgeBeat(double ErrorSec, double WindowScale = 1.0)
	{
		const double E = std::fabs(ErrorSec);
		if (E <= PerfectWindowSec * WindowScale)
		{
			return Perfect;
		}
		if (E <= GoodWindowSec * WindowScale)
		{
			return Good;
		}
		return Miss;
	}

	// 샤리 쥐기: 박자보다 일찍 누르면(서두르면) 너무 세게, 늦으면 너무 약하게 쥔다.
	enum EShariGrade { TooWeak = 0, Weak = 1, ShariPerfect = 2, Strong = 3, TooStrong = 4 };

	inline EShariGrade GradeShari(double MeanSignedErrorSec)
	{
		if (MeanSignedErrorSec < -0.15) return TooStrong;
		if (MeanSignedErrorSec < -0.07) return Strong;
		if (MeanSignedErrorSec > 0.15) return TooWeak;
		if (MeanSignedErrorSec > 0.07) return Weak;
		return ShariPerfect;
	}

	inline double ShariScore(double MeanAbsErrorSec)
	{
		double S = 1.0 - MeanAbsErrorSec / 0.25;
		S = S < 0.0 ? 0.0 : (S > 1.0 ? 1.0 : S);
		return 100.0 * S;
	}

	// ── 와사비 룰렛 (기획서 5-4) ───────────────────────────────────
	enum EWasabiZone { Bland = 0, WasabiPerfect = 1, Steam = 2, FireBreath = 3, Rocket = 4 };

	inline EWasabiZone WasabiZoneOf(double Amount01)
	{
		if (Amount01 < 0.10) return Bland;
		if (Amount01 < 0.35) return WasabiPerfect;
		if (Amount01 < 0.60) return Steam;
		if (Amount01 < 0.85) return FireBreath;
		return Rocket;
	}

	// 바늘이 0→1→0 으로 왕복하는 삼각파.
	inline double WasabiNeedle(double TimeSec, double PeriodSec)
	{
		const double Phase = std::fmod(TimeSec, PeriodSec) / PeriodSec;
		return Phase < 0.5 ? Phase * 2.0 : 2.0 - Phase * 2.0;
	}

	// bThrillSeeker: 로켓 발사를 원하는 손님(날다람G). 이 손님은 85% 이상 넣어야 만점.
	inline double WasabiScore(double Amount01, bool bThrillSeeker)
	{
		const EWasabiZone Zone = WasabiZoneOf(Amount01);
		if (bThrillSeeker)
		{
			return Zone == Rocket ? 100.0 : 20.0 + 60.0 * Amount01;
		}
		switch (Zone)
		{
		case WasabiPerfect:
		{
			// 구간 한가운데(22.5%)에서 100점, 가장자리에서 80점
			const double Off = std::fabs(Amount01 - 0.225) / 0.125;
			return 100.0 - 20.0 * Off;
		}
		case Bland:      return 40.0;
		case Steam:      return 60.0;
		case FireBreath: return 30.0;
		default:         return 10.0;
		}
	}

	// ── 접시 점수 (기획서 5-8) ────────────────────────────────────
	inline double PlateScore(double Quality, double Shari, double Wasabi, double Plating, double TasteMatch, double FreshnessPercent)
	{
		const double Raw = Quality * 0.35 + Shari * 0.20 + Wasabi * 0.15 + Plating * 0.20 + TasteMatch * 0.10;
		return Raw * FreshnessMultiplier(FreshnessPercent);
	}

	// 손님 취향 레이더 [매운맛, 단맛, 모험심, 비주얼, 양] 과 초밥 값의 닮은 정도.
	inline double CosineSimilarity(const double A[5], const double B[5])
	{
		double Dot = 0.0, NA = 0.0, NB = 0.0;
		for (int I = 0; I < 5; ++I)
		{
			Dot += A[I] * B[I];
			NA += A[I] * A[I];
			NB += B[I] * B[I];
		}
		if (NA <= 0.0 || NB <= 0.0)
		{
			return 0.0;
		}
		return Dot / (std::sqrt(NA) * std::sqrt(NB));
	}

	inline int StarsFromScore(double Score)
	{
		if (Score >= 90.0) return 5;
		if (Score >= 75.0) return 4;
		if (Score >= 60.0) return 3;
		if (Score >= 40.0) return 2;
		return 1;
	}

	// ── 보이지 않는 할배의 손 (기획서 7-4) ─────────────────────────
	inline double DdaWindowBonus(int ConsecutiveFails)
	{
		if (ConsecutiveFails >= 5) return 0.10;
		if (ConsecutiveFails >= 3) return 0.05;
		return 0.0;
	}
}
