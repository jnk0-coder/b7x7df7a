// 재료 도감 데이터 (프로토타입은 코드 안 표로 둔다. 정식판은 DataTable 로 옮길 것 — 기획서 8-2).
#pragma once

#include "CoreMinimal.h"
#include "SushiTypes.h"

// 할배의 손(연속 실패 보정)이 미션별로 실패 횟수를 센다 (기획서 7-4).
namespace SushiMissions
{
	inline const FName EggHeist(TEXT("EggHeist"));
	inline const FName Fishing(TEXT("Fishing"));
	inline const FName Shari(TEXT("Shari"));
	inline const FName Cooking(TEXT("Cooking"));
}

namespace SushiCatalog
{
	inline FLinearColor RarityColor(ESushiRarity Rarity)
	{
		switch (Rarity)
		{
		case ESushiRarity::Rare:   return FLinearColor(0.35f, 0.6f, 1.f);
		case ESushiRarity::Hero:   return FLinearColor(0.75f, 0.4f, 1.f);
		case ESushiRarity::Legend: return FLinearColor(1.f, 0.8f, 0.2f);
		case ESushiRarity::Meme:   return FLinearColor(1.f, 0.45f, 0.75f);
		default:                   return FLinearColor::White;
		}
	}

	inline FText RarityName(ESushiRarity Rarity)
	{
		switch (Rarity)
		{
		case ESushiRarity::Rare:   return NSLOCTEXT("Sushi", "Rare", "희귀");
		case ESushiRarity::Hero:   return NSLOCTEXT("Sushi", "Hero", "영웅");
		case ESushiRarity::Legend: return NSLOCTEXT("Sushi", "Legend", "전설");
		case ESushiRarity::Meme:   return NSLOCTEXT("Sushi", "Meme", "밈급");
		default:                   return NSLOCTEXT("Sushi", "Common", "일반");
		}
	}

	// 손님이 내는 돈 배율.
	inline float RarityCoinMultiplier(ESushiRarity Rarity)
	{
		switch (Rarity)
		{
		case ESushiRarity::Rare:   return 1.5f;
		case ESushiRarity::Hero:   return 2.5f;
		case ESushiRarity::Legend: return 5.f;
		case ESushiRarity::Meme:   return 10.f;
		default:                   return 1.f;
		}
	}

	// 꼬꼬레오네 둥지에서 훔친 알. 등급은 SushiBalance::RollRarity 로 정해진다.
	inline FSushiIngredient MakeEgg(ESushiRarity Rarity, float Quality, double NowSeconds)
	{
		FSushiIngredient Egg;
		Egg.Rarity = Rarity;
		Egg.CatchStyle = ESushiCatchStyle::Stealth;
		Egg.Quality = Quality;
		Egg.HalfLifeMinutes = 30.f;
		Egg.AcquiredWorldSeconds = NowSeconds;
		switch (Rarity)
		{
		case ESushiRarity::Rare:
			Egg.Id = TEXT("egg_double");
			Egg.DisplayName = NSLOCTEXT("Sushi", "EggRare", "노른자 두 개 쌍란");
			Egg.Flavor = FSushiFlavor(0.f, 0.9f, 0.4f, 0.6f, 0.8f);
			break;
		case ESushiRarity::Hero:
			Egg.Id = TEXT("egg_crest");
			Egg.DisplayName = NSLOCTEXT("Sushi", "EggHero", "꼬꼬레오네 가문 문장 달걀");
			Egg.Flavor = FSushiFlavor(0.1f, 0.8f, 0.6f, 0.9f, 0.5f);
			break;
		case ESushiRarity::Legend:
			Egg.Id = TEXT("egg_goldie");
			Egg.DisplayName = NSLOCTEXT("Sushi", "EggLegend", "골디의 황금알");
			Egg.Flavor = FSushiFlavor(0.1f, 0.7f, 0.9f, 1.f, 0.5f);
			break;
		case ESushiRarity::Meme:
			Egg.Id = TEXT("egg_meme_clue");
			Egg.DisplayName = NSLOCTEXT("Sushi", "EggMeme", "'초밥이 되고 싶은 초밥'의 단서가 묻은 달걀");
			Egg.Flavor = FSushiFlavor(0.5f, 0.5f, 1.f, 0.7f, 0.5f);
			break;
		default:
			Egg.Id = TEXT("egg");
			Egg.DisplayName = NSLOCTEXT("Sushi", "EggCommon", "평범한 달걀");
			Egg.Flavor = FSushiFlavor(0.f, 0.9f, 0.2f, 0.5f, 0.5f);
			break;
		}
		return Egg;
	}

	// 리듬 낚시 생선 표 (기획서 4-2).
	struct FFishInfo
	{
		const TCHAR* Id;
		FText Name;
		FText Genre;
		float Bpm;
		float HalfLifeMinutes;
		FSushiFlavor Flavor;
	};

	inline int32 NumFish() { return 5; }

	inline FFishInfo GetFish(int32 Index)
	{
		switch (Index)
		{
		case 0:  return { TEXT("flatfish"), NSLOCTEXT("Sushi", "Flatfish", "광어"), NSLOCTEXT("Sushi", "Ballad", "발라드"), 70.f, 10.f, FSushiFlavor(0.1f, 0.4f, 0.2f, 0.7f, 0.4f) };
		case 1:  return { TEXT("mackerel"), NSLOCTEXT("Sushi", "Mackerel", "고등어"), NSLOCTEXT("Sushi", "Trot", "트로트"), 130.f, 7.f, FSushiFlavor(0.3f, 0.2f, 0.4f, 0.4f, 0.6f) };
		case 2:  return { TEXT("salmon"), NSLOCTEXT("Sushi", "Salmon", "연어"), NSLOCTEXT("Sushi", "KPop", "K-POP"), 120.f, 9.f, FSushiFlavor(0.1f, 0.6f, 0.3f, 0.8f, 0.6f) };
		case 3:  return { TEXT("eel"), NSLOCTEXT("Sushi", "Eel", "장어"), NSLOCTEXT("Sushi", "Edm", "EDM"), 160.f, 12.f, FSushiFlavor(0.2f, 0.8f, 0.5f, 0.6f, 0.8f) };
		default: return { TEXT("puffer"), NSLOCTEXT("Sushi", "Puffer", "복어"), NSLOCTEXT("Sushi", "Reggae", "레게"), 90.f, 8.f, FSushiFlavor(0.4f, 0.2f, 1.f, 0.6f, 0.3f) };
		}
	}

	inline FSushiIngredient MakeFish(int32 Index, float Quality, double NowSeconds)
	{
		const FFishInfo Info = GetFish(Index);
		FSushiIngredient Fish;
		Fish.Id = Info.Id;
		Fish.DisplayName = Info.Name;
		Fish.Rarity = ESushiRarity::Common;
		Fish.CatchStyle = ESushiCatchStyle::Rhythm;
		Fish.Quality = Quality;
		Fish.HalfLifeMinutes = Info.HalfLifeMinutes;
		Fish.AcquiredWorldSeconds = NowSeconds;
		Fish.Flavor = Info.Flavor;
		return Fish;
	}

	// 잡는 방식 품질 보정 (기획서 4-1 표).
	inline float CatchStyleQualityBonus(ESushiCatchStyle Style)
	{
		switch (Style)
		{
		case ESushiCatchStyle::Stealth: return 20.f;
		case ESushiCatchStyle::Rhythm:  return 10.f;
		case ESushiCatchStyle::Brawl:   return -10.f;
		default:                        return 0.f;
		}
	}
}
