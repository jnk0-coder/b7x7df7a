// 밸런스 수식이 기획서 숫자와 맞는지 확인하는 테스트 (Unreal 없이 실행).
//   g++ -std=c++17 -I Source/SushiWild/Public Tests/BalanceTest.cpp -o /tmp/balance_test && /tmp/balance_test
#include "SushiBalance.h"

#include <cstdio>
#include <cstdlib>
#include <random>

using namespace SushiBalance;

static int Failures = 0;

static void Check(bool bOk, const char* What, double Got)
{
	std::printf("[%s] %-48s %.4f\n", bOk ? " OK " : "FAIL", What, Got);
	if (!bOk)
	{
		++Failures;
	}
}

static bool Near(double A, double B, double Tol) { return std::fabs(A - B) <= Tol; }

int main()
{
	// 4-1 신선도 표
	Check(Near(Freshness(5, 6), 56.1, 0.1), "shrimp freshness after 5 min = 56%", Freshness(5, 6));
	Check(Near(Freshness(10, 8), 42.0, 0.1), "tuna freshness after 10 min = 42%", Freshness(10, 8));
	Check(Near(Freshness(10, 30), 79.4, 0.1), "egg freshness after 10 min = 79%", Freshness(10, 30));
	Check(Freshness(999, 0) == 100.0, "rice never spoils", Freshness(999, 0));
	Check(Near(FreshnessMultiplier(0), 0.5, 1e-9), "freshness 0% halves the score", FreshnessMultiplier(0));

	// 3-2 등급 확률과 천장
	Check(Near(ProbAtLeastOne(0.025, 40), 0.637, 0.001), "legend within 40 tries = 64%", ProbAtLeastOne(0.025, 40));
	Check(Near(1.0 - ProbAtLeastOne(0.025, 99), 0.0816, 0.0005), "no legend in 99 tries = 8%", 1.0 - ProbAtLeastOne(0.025, 99));
	Check(Near(1.0 - ProbAtLeastOne(0.09, 29), 0.0649, 0.0005), "no hero in 29 tries = 6.5%", 1.0 - ProbAtLeastOne(0.09, 29));
	Check(Near(1.0 - ProbAtLeastOne(1.0 / 6.0, 10), 0.1615, 0.0005), "dice: no six in 10 throws = 16%", 1.0 - ProbAtLeastOne(1.0 / 6.0, 10));

	{
		std::mt19937_64 Rng(42);
		std::uniform_real_distribution<double> Uni(0.0, 1.0);
		FPityState Pity;
		const int N = 2000000;
		int Count[5] = { 0, 0, 0, 0, 0 };
		int LegendGap = 0, MaxLegendGap = 0, HeroGap = 0, MaxHeroGap = 0;
		for (int I = 0; I < N; ++I)
		{
			const ERarity R = RollRarity(Uni(Rng), Pity);
			++Count[R];
			LegendGap = (R == Legend) ? 0 : LegendGap + 1;
			HeroGap = (R == Hero || R == Legend) ? 0 : HeroGap + 1;
			MaxLegendGap = LegendGap > MaxLegendGap ? LegendGap : MaxLegendGap;
			MaxHeroGap = HeroGap > MaxHeroGap ? HeroGap : MaxHeroGap;
		}
		Check(MaxLegendGap <= LegendPity - 1, "legend pity: never 100 misses in a row", MaxLegendGap);
		Check(MaxHeroGap <= HeroPity - 1, "hero pity: never 30 misses in a row", MaxHeroGap);
		const double LegendRate = double(Count[Legend]) / N;
		// 천장이 있으면 실제 전설 비율은 1 / E[대기] = 0.025 / (1 - 0.975^100) ≈ 2.72%
		Check(Near(LegendRate, 0.0272, 0.0005), "legend rate with pity ~= 2.72%", LegendRate);
		Check(Near(double(Count[Meme]) / N, 0.005, 0.0003), "meme clue rate ~= 0.5%", double(Count[Meme]) / N);
	}

	// 3-4 날씨
	{
		double Pi[4];
		StationaryWeather(Pi);
		Check(Near(Pi[Clear], 0.397, 0.001), "long-run clear = 40%", Pi[Clear]);
		Check(Near(Pi[Cloudy], 0.306, 0.001), "long-run cloudy = 31%", Pi[Cloudy]);
		Check(Near(Pi[Rain], 0.208, 0.001), "long-run rain = 21%", Pi[Rain]);
		Check(Near(Pi[Fog], 0.089, 0.001), "long-run fog = 9%", Pi[Fog]);

		// 정상분포에서 시작하면(아주 먼 미래) 밤 창에 비 올 확률 ≈ 33%
		double NoRain = 0.0;
		for (int W = 0; W < 4; ++W)
		{
			if (W != Rain)
			{
				NoRain += Pi[W] * (1.0 - WeatherTransition[W][Rain]);
			}
		}
		const double RainyNight = 1.0 - NoRain;
		Check(Near(RainyNight, 0.333, 0.001), "rainy night window = 33%", RainyNight);
		Check(Near(RainChanceInNightWindow(Clear, 500), RainyNight, 1e-6), "forecast far ahead == long-run", RainChanceInNightWindow(Clear, 500));
		Check(Near(RainChanceInNightWindow(Rain, 0), 1.0, 1e-9), "already raining in window = 100%", RainChanceInNightWindow(Rain, 0));

		const double SquidPerDay = RainyNight * 0.35;
		Check(Near(1.0 / SquidPerDay, 8.57, 0.01), "legend squid every ~8.6 days", 1.0 / SquidPerDay);
		Check(Near(ProbAtLeastOne(SquidPerDay, 7), 0.580, 0.001), "squid within 7 days = 58%", ProbAtLeastOne(SquidPerDay, 7));

		int Stuck = 0;
		std::mt19937_64 Rng(7);
		std::uniform_real_distribution<double> Uni(0.0, 1.0);
		EWeather W = Clear;
		int RainCount = 0;
		const int Steps = 1000000;
		for (int I = 0; I < Steps; ++I)
		{
			W = NextWeather(W, Uni(Rng));
			RainCount += (W == Rain);
			Stuck += (W < Clear || W > Fog);
		}
		Check(Stuck == 0 && Near(double(RainCount) / Steps, Pi[Rain], 0.003), "simulated rain share matches", double(RainCount) / Steps);
	}

	// 6-2 돌발 이벤트
	Check(Near(PoissonPmf(0, 3.0), 0.0498, 0.0001), "no event in a day = 5%", PoissonPmf(0, 3.0));
	{
		double AtMost4 = 0.0;
		for (int K = 0; K <= 4; ++K)
		{
			AtMost4 += PoissonPmf(K, 3.0);
		}
		Check(Near(1.0 - AtMost4, 0.1847, 0.0001), "5+ events in a day = 18%", 1.0 - AtMost4);
	}

	// 5-3, 5-4 미니게임 판정
	Check(JudgeBeat(0.05) == Perfect && JudgeBeat(-0.1) == Good && JudgeBeat(0.2) == Miss, "beat judge windows", 0);
	Check(JudgeBeat(0.075, 1.10) == Perfect, "DDA widens the window", DdaWindowBonus(5));
	Check(GradeShari(-0.2) == TooStrong && GradeShari(0.0) == ShariPerfect && GradeShari(0.2) == TooWeak, "shari grades", 0);
	Check(WasabiZoneOf(0.05) == Bland && WasabiZoneOf(0.2) == WasabiPerfect && WasabiZoneOf(0.9) == Rocket, "wasabi zones", 0);
	Check(Near(WasabiNeedle(0.5, 2.0), 0.5, 1e-9) && Near(WasabiNeedle(1.0, 2.0), 1.0, 1e-9) && Near(WasabiNeedle(2.0, 2.0), 0.0, 1e-9), "wasabi needle triangle wave", WasabiNeedle(1.0, 2.0));
	Check(Near(WasabiScore(0.225, false), 100.0, 1e-9) && WasabiScore(0.95, true) == 100.0, "wasabi score peaks", WasabiScore(0.225, false));

	// 5-8 채점
	{
		const double A[5] = { 1, 0, 0, 0, 0 };
		const double B[5] = { 2, 0, 0, 0, 0 };
		const double C[5] = { 0, 1, 0, 0, 0 };
		Check(Near(CosineSimilarity(A, B), 1.0, 1e-9) && Near(CosineSimilarity(A, C), 0.0, 1e-9), "cosine similarity", CosineSimilarity(A, B));
		Check(Near(PlateScore(100, 100, 100, 100, 100, 100), 100.0, 1e-9), "perfect plate = 100", PlateScore(100, 100, 100, 100, 100, 100));
		Check(Near(PlateScore(100, 100, 100, 100, 100, 0), 50.0, 1e-9), "rotten plate = 50", PlateScore(100, 100, 100, 100, 100, 0));
		Check(StarsFromScore(90) == 5 && StarsFromScore(74.9) == 3 && StarsFromScore(10) == 1, "star thresholds", 0);
	}

	std::printf("\n%s (%d failure%s)\n", Failures == 0 ? "ALL PASSED" : "FAILED", Failures, Failures == 1 ? "" : "s");
	return Failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
