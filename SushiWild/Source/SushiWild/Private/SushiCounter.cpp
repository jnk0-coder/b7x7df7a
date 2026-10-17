#include "SushiCounter.h"

#include "Engine/Canvas.h"
#include "Kismet/GameplayStatics.h"
#include "SushiCatalog.h"
#include "SushiChefCharacter.h"
#include "SushiCustomer.h"
#include "SushiHUD.h"
#include "SushiShapes.h"
#include "SushiWildGameState.h"

#define LOCTEXT_NAMESPACE "SushiCounter"

namespace
{
	constexpr double ShariLateLimit = 0.3; // 이만큼 늦으면 그 박자는 놓친 것 (너무 약하게 쥔 걸로 친다)
	const FVector SeatOffsets[] = { FVector(180.f, -120.f, 0.f), FVector(180.f, 120.f, 0.f) };
	const FLinearColor Cardboard(0.78f, 0.62f, 0.42f);
}

ASushiCounter::ASushiCounter()
{
	PrimaryActorTick.bCanEverTick = true;
	InteractRadius = 280.f;
	Seats.SetNum(NumSeats);
}

void ASushiCounter::BeginPlay()
{
	Super::BeginPlay();
	BuildVisuals();
}

void ASushiCounter::BuildVisuals()
{
	// 카운터 (부딪힘 있음) + 도마 + 밥통 + 와사비 그릇
	SushiShapes::Add(this, nullptr, ESushiShape::Cube, FVector(0.f, 0.f, 50.f), FVector(0.8f, 3.6f, 1.f), FLinearColor(0.5f, 0.32f, 0.18f), true);
	SushiShapes::Add(this, nullptr, ESushiShape::Cube, FVector(0.f, 0.f, 103.f), FVector(0.9f, 3.7f, 0.06f), FLinearColor(0.85f, 0.7f, 0.5f));
	SushiShapes::Add(this, nullptr, ESushiShape::Cube, FVector(-10.f, 0.f, 108.f), FVector(0.5f, 0.8f, 0.04f), FLinearColor(0.95f, 0.88f, 0.7f));
	SushiShapes::Add(this, nullptr, ESushiShape::Cylinder, FVector(-10.f, -110.f, 125.f), FVector(0.45f, 0.45f, 0.4f), FLinearColor(0.95f, 0.95f, 0.95f));
	SushiShapes::Add(this, nullptr, ESushiShape::Sphere, FVector(-10.f, 110.f, 115.f), FVector(0.25f), FLinearColor(0.4f, 0.8f, 0.2f));

	// 골판지 포장마차: 기둥 4개 + 지붕 + 노렌(가림막)
	for (const FVector& Post : { FVector(-160.f, -230.f, 0.f), FVector(-160.f, 230.f, 0.f), FVector(60.f, -230.f, 0.f), FVector(60.f, 230.f, 0.f) })
	{
		SushiShapes::Add(this, nullptr, ESushiShape::Cylinder, Post + FVector(0.f, 0.f, 125.f), FVector(0.15f, 0.15f, 2.5f), Cardboard);
	}
	SushiShapes::Add(this, nullptr, ESushiShape::Cube, FVector(-50.f, 0.f, 255.f), FVector(2.8f, 5.4f, 0.08f), Cardboard, false, FRotator(0.f, 0.f, 0.f));
	SushiShapes::Add(this, nullptr, ESushiShape::Cube, FVector(60.f, 0.f, 220.f), FVector(0.02f, 4.4f, 0.5f), FLinearColor(0.1f, 0.15f, 0.4f));

	for (const FVector& Seat : SeatOffsets)
	{
		SushiShapes::Add(this, nullptr, ESushiShape::Cylinder, Seat + FVector(-35.f, 0.f, 25.f), FVector(0.4f, 0.4f, 0.5f), FLinearColor(0.6f, 0.15f, 0.15f));
	}
}

void ASushiCounter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	TrySpawnCustomer(DeltaSeconds);

	const double Now = GetWorld()->GetTimeSeconds();
	switch (Phase)
	{
	case ESushiCookPhase::Shari:
		// 지나가 버린 박자는 '늦게 쥔 것'으로 처리
		while (ShariNext < ShariBeats && Now - ShariBeatTime(ShariNext) > ShariLateLimit)
		{
			ShariErrors[ShariNext++] = static_cast<float>(ShariLateLimit);
		}
		if (ShariNext >= ShariBeats)
		{
			FinishShari();
		}
		break;

	case ESushiCookPhase::Result:
		ResultTimeLeft -= DeltaSeconds;
		if (ResultTimeLeft <= 0.f)
		{
			EndCooking();
		}
		break;

	default:
		break;
	}
}

void ASushiCounter::TrySpawnCustomer(float DeltaSeconds)
{
	// 떠난 손님 자리는 비운다 (단, 지금 요리 대상은 대접 전까지 유지)
	for (TObjectPtr<ASushiCustomer>& Seat : Seats)
	{
		if (Seat && (!IsValid(Seat) || !Seat->IsWaiting()) && Seat != Target.Get())
		{
			Seat = nullptr;
		}
	}

	SpawnTimer -= DeltaSeconds;
	if (SpawnTimer > 0.f)
	{
		return;
	}

	ASushiWildGameState* State = GetWorld()->GetGameState<ASushiWildGameState>();
	if (!State)
	{
		return;
	}
	SpawnTimer = State->GetRng().FRandRange(CustomerIntervalMin, CustomerIntervalMax);

	for (int32 i = 0; i < Seats.Num(); ++i)
	{
		if (Seats[i])
		{
			continue;
		}
		const float Roll = State->GetRng().FRand();
		const ESushiCustomerKind Kind = Roll < 0.2f ? ESushiCustomerKind::Thrill : (Roll < 0.4f ? ESushiCustomerKind::Picky : ESushiCustomerKind::Regular);

		const FTransform SeatTransform(FRotator::ZeroRotator, GetActorLocation() + SeatOffsets[i]);
		ASushiCustomer* Customer = GetWorld()->SpawnActorDeferred<ASushiCustomer>(ASushiCustomer::StaticClass(), SeatTransform);
		if (!Customer)
		{
			return;
		}
		Customer->InitCustomer(Kind, State->GetRng());
		Customer->FinishSpawning(SeatTransform);
		Seats[i] = Customer;
		State->PushToast(FText::Format(LOCTEXT("NewCustomer", "손님 입장: {0}"), Customer->GetDisplayName()), FLinearColor(0.9f, 0.9f, 1.f), 2.5f);
		return; // 한 번에 한 명씩
	}
}

ASushiCustomer* ASushiCounter::FindWaitingCustomer() const
{
	ASushiCustomer* Best = nullptr;
	for (const TObjectPtr<ASushiCustomer>& Seat : Seats)
	{
		// 가장 오래 기다린(인내심이 적게 남은) 손님부터
		if (IsValid(Seat) && Seat->IsWaiting() && (!Best || Seat->GetPatience01() < Best->GetPatience01()))
		{
			Best = Seat;
		}
	}
	return Best;
}

FText ASushiCounter::GetPrompt(const ASushiChefCharacter* Chef) const
{
	switch (Phase)
	{
	case ESushiCookPhase::Shari:  return LOCTEXT("PromptShari", "박자에 맞춰 [E] — 쥐기·돌리기·쥐기·톡!");
	case ESushiCookPhase::Wasabi: return LOCTEXT("PromptWasabi", "[E] 와사비 바늘 멈추기");
	case ESushiCookPhase::Result: return LOCTEXT("PromptResult", "[E] 계속");
	default: break;
	}

	const ASushiCustomer* Customer = FindWaitingCustomer();
	if (!Customer)
	{
		return LOCTEXT("PromptNoCustomer", "손님 기다리는 중… (재료 구하러 다녀와도 돼유)");
	}
	if (!Chef || Chef->GetInventory().Num() == 0)
	{
		return LOCTEXT("PromptNoIngredient", "재료가 없슈! 농장(서쪽)에서 알을 훔치거나 해변(동쪽)에서 낚시해유.");
	}
	return FText::Format(LOCTEXT("PromptCook", "[E] 초밥 만들기 — {0}"), Customer->GetDisplayName());
}

void ASushiCounter::OnInteractPressed(ASushiChefCharacter* Chef)
{
	const double Now = GetWorld()->GetTimeSeconds();
	switch (Phase)
	{
	case ESushiCookPhase::Idle:   StartCooking(Chef); break;
	case ESushiCookPhase::Shari:  PressShari(Now); break;
	case ESushiCookPhase::Wasabi: PressWasabi(Now); break;
	case ESushiCookPhase::Result: EndCooking(); break;
	}
}

void ASushiCounter::StartCooking(ASushiChefCharacter* Chef)
{
	ASushiCustomer* Customer = FindWaitingCustomer();
	if (!Chef || !Customer || !Chef->TakeBestIngredient(Ingredient))
	{
		return;
	}

	Cook = Chef;
	Target = Customer;
	Chef->LockToMinigame(this);

	Phase = ESushiCookPhase::Shari;
	ShariStart = GetWorld()->GetTimeSeconds() + 2.0 * (60.0 / ShariBpm);
	ShariNext = 0;

	if (ASushiWildGameState* State = GetWorld()->GetGameState<ASushiWildGameState>())
	{
		State->PushToast(FText::Format(LOCTEXT("Start", "[{0}] {1} 로 초밥을 만들어유. 신선도 {2}%"),
			SushiCatalog::RarityName(Ingredient.Rarity), Ingredient.DisplayName,
			FText::AsNumber(FMath::RoundToInt(Chef->GetFreshness(Ingredient)))), SushiCatalog::RarityColor(Ingredient.Rarity));
	}
}

void ASushiCounter::PressShari(double Now)
{
	if (ShariNext >= ShariBeats)
	{
		return;
	}
	const double Error = Now - ShariBeatTime(ShariNext);
	if (Error < -ShariLateLimit)
	{
		return; // 너무 이른 건 무시
	}
	ShariErrors[ShariNext++] = static_cast<float>(FMath::Clamp(Error, -ShariLateLimit, ShariLateLimit));
	if (ShariNext >= ShariBeats)
	{
		FinishShari();
	}
}

void ASushiCounter::FinishShari()
{
	ASushiWildGameState* State = GetWorld()->GetGameState<ASushiWildGameState>();
	// 연속 실패 중이면 할배가 몰래 판정을 넉넉하게 (기획서 7-4)
	const float Scale = State ? State->GetJudgeWindowScale(SushiMissions::Shari) : 1.f;

	double SumSigned = 0.0;
	double SumAbs = 0.0;
	for (const float E : ShariErrors)
	{
		SumSigned += E / Scale;
		SumAbs += FMath::Abs(E) / Scale;
	}
	ShariGrade = SushiBalance::GradeShari(SumSigned / ShariBeats);
	ShariScoreValue = static_cast<float>(SushiBalance::ShariScore(SumAbs / ShariBeats));

	if (State)
	{
		State->RegisterMissionResult(SushiMissions::Shari, ShariScoreValue >= 60.f);
		FText Reaction;
		switch (ShariGrade)
		{
		case SushiBalance::TooStrong: Reaction = LOCTEXT("TooStrong", "너무 세게 쥐었슈! 밥이 돌덩이가 됐어유."); break;
		case SushiBalance::Strong:    Reaction = LOCTEXT("Strong", "좀 세게 쥐었슈. 떡이 됐어유."); break;
		case SushiBalance::Weak:      Reaction = LOCTEXT("Weak", "살살 쥐었더니 푸석해유."); break;
		case SushiBalance::TooWeak:   Reaction = LOCTEXT("TooWeak", "밥알 공중분해! 쌀 비가 내려유."); break;
		default:                      Reaction = LOCTEXT("ShariPerfect", "Perfect! 밥알들이 아카펠라로 '아~♪'"); break;
		}
		State->PushToast(FText::Format(LOCTEXT("ShariDone", "{0} (샤리 {1}점)"), Reaction, FText::AsNumber(FMath::RoundToInt(ShariScoreValue))), FLinearColor(1.f, 1.f, 0.85f));

		bRussianWasabi = State->GetRng().FRand() < RussianWasabiChance;
	}

	// 귀한 재료일수록 바늘이 빠르다
	WasabiPeriod = Ingredient.Rarity == ESushiRarity::Legend ? 1.f : (Ingredient.Rarity == ESushiRarity::Hero ? 1.3f : WasabiPeriodSeconds);
	WasabiStart = GetWorld()->GetTimeSeconds();
	Phase = ESushiCookPhase::Wasabi;
}

float ASushiCounter::CurrentWasabiNeedle(double Now) const
{
	return static_cast<float>(SushiBalance::WasabiNeedle(Now - WasabiStart, WasabiPeriod));
}

void ASushiCounter::PressWasabi(double Now)
{
	const float Shown = CurrentWasabiNeedle(Now);
	// 러시안 와사비: 보이는 것보다 1.6배 맵다
	WasabiAmount = bRussianWasabi ? FMath::Min(1.f, Shown * 1.6f) : Shown;
	Serve();
}

void ASushiCounter::Serve()
{
	ASushiWildGameState* State = GetWorld()->GetGameState<ASushiWildGameState>();
	ASushiChefCharacter* Chef = Cook.Get();
	ASushiCustomer* Customer = Target.Get();

	if (!State || !IsValid(Customer) || !Customer->IsWaiting())
	{
		if (State)
		{
			State->PushToast(LOCTEXT("CustomerGone", "손님이 가버렸슈… 초밥은 그냥 내가 먹었슈."), FLinearColor(0.8f, 0.8f, 0.8f));
		}
		EndCooking();
		return;
	}

	const bool bThrill = Customer->GetKind() == ESushiCustomerKind::Thrill;
	const float Freshness = Chef ? Chef->GetFreshness(Ingredient) : 100.f;
	const bool bSpoiled = Freshness < 5.f;

	// 손님 취향 레이더 vs 초밥 맛 (와사비는 매운맛 축을 올린다)
	double TasteRadar[5];
	double Dish[5];
	Customer->GetTaste().ToArray(TasteRadar);
	FSushiFlavor DishFlavor = Ingredient.Flavor;
	DishFlavor.Spicy = FMath::Max(DishFlavor.Spicy, WasabiAmount);
	DishFlavor.ToArray(Dish);
	const double TasteMatch = 100.0 * FMath::Max(0.0, SushiBalance::CosineSimilarity(TasteRadar, Dish));

	const double WasabiPoints = SushiBalance::WasabiScore(WasabiAmount, bThrill);
	ResultScore = static_cast<float>(SushiBalance::PlateScore(Ingredient.Quality, ShariScoreValue, WasabiPoints, PlatingScore, TasteMatch, Freshness));
	ResultStars = bSpoiled ? 1 : SushiBalance::StarsFromScore(ResultScore);
	ResultCoins = FMath::RoundToInt(ResultScore * SushiCatalog::RarityCoinMultiplier(Ingredient.Rarity));

	State->AddCoins(ResultCoins);
	State->ApplyReview(ResultStars);
	State->RegisterMissionResult(SushiMissions::Cooking, ResultStars >= 3);

	const SushiBalance::EWasabiZone Zone = SushiBalance::WasabiZoneOf(WasabiAmount);
	FText Reaction;
	switch (Zone)
	{
	case SushiBalance::Bland:         Reaction = LOCTEXT("Bland", "손님 하품… 밍밍하대유."); break;
	case SushiBalance::WasabiPerfect: Reaction = LOCTEXT("WasabiPerfect", "눈물 한 방울. \"이것이… 인생…\""); break;
	case SushiBalance::Steam:         Reaction = LOCTEXT("Steam", "콧구멍에서 증기기관차 '뿌~'"); break;
	case SushiBalance::FireBreath:    Reaction = LOCTEXT("Fire", "입에서 불을 뿜었슈! 노렌에 불 붙을 뻔."); break;
	default:
		Reaction = bThrill ? LOCTEXT("RocketThrill", "날다람G: 이거여!! 로켓 발사~~!") : LOCTEXT("Rocket", "손님 로켓 발사!! 천장 뚫고 하늘로~");
		break;
	}
	if (bRussianWasabi)
	{
		State->PushToast(LOCTEXT("Russian", "…러시안 와사비였슈! 보이는 것보다 훨씬 매웠어유."), FLinearColor(0.5f, 1.f, 0.4f));
	}
	State->PushToast(Reaction, FLinearColor(0.6f, 1.f, 0.5f), 4.f);
	if (bSpoiled)
	{
		State->PushToast(LOCTEXT("Spoiled", "꾸르륵… 상한 재료였슈. 화장실 대기줄 발생! (별점 1)"), FLinearColor(0.7f, 0.6f, 0.3f), 4.f);
	}
	State->PushToast(FText::Format(LOCTEXT("Score", "{0}점 · 별점 {1}/5 · +{2}코인"),
		FText::AsNumber(FMath::RoundToInt(ResultScore)), FText::AsNumber(ResultStars), FText::AsNumber(ResultCoins)), FLinearColor(1.f, 0.9f, 0.4f), 4.f);

	if (Zone == SushiBalance::Rocket)
	{
		Customer->LaunchRocket(State->GetRng());
	}
	else
	{
		Customer->LeaveHappy();
	}

	Phase = ESushiCookPhase::Result;
	ResultTimeLeft = 2.5f;
}

void ASushiCounter::EndCooking()
{
	Phase = ESushiCookPhase::Idle;
	if (ASushiChefCharacter* Chef = Cook.Get())
	{
		Chef->UnlockMinigame(this);
	}
	Cook.Reset();
	Target.Reset();
}

void ASushiCounter::DrawMinigameHUD(ASushiHUD* HUD, UCanvas* Canvas) const
{
	if (!HUD || !Canvas)
	{
		return;
	}
	const double Now = GetWorld()->GetTimeSeconds();
	const float W = 640.f;
	const float X = (Canvas->ClipX - W) * 0.5f;
	const float Y = Canvas->ClipY * 0.6f;
	HUD->DrawPanel(X - 16.f, Y - 70.f, W + 32.f, 190.f);

	if (Phase == ESushiCookPhase::Shari)
	{
		HUD->DrawLabel(LOCTEXT("ShariTitle", "샤리 쥐기 — 쥐기 · 돌리기 · 쥐기 · 톡!"), X, Y - 62.f, FLinearColor::White, 20);
		HUD->DrawLabel(LOCTEXT("ShariHint", "일찍 누르면 너무 세게(돌덩이), 늦게 누르면 너무 약하게(공중분해)"), X, Y - 36.f, FLinearColor(0.8f, 0.8f, 0.8f), 15);

		const float LaneH = 60.f;
		const float HitX = X + 60.f;
		const float PixelsPerSecond = 300.f;
		HUD->DrawRect(FLinearColor(0.25f, 0.18f, 0.1f, 0.85f), X, Y, W, LaneH);
		HUD->DrawRect(FLinearColor(1.f, 1.f, 1.f, 0.9f), HitX - 3.f, Y - 6.f, 6.f, LaneH + 12.f);
		static const FText BeatNames[ShariBeats] = {
			LOCTEXT("Beat0", "쥐기"), LOCTEXT("Beat1", "돌리기"), LOCTEXT("Beat2", "쥐기"), LOCTEXT("Beat3", "톡!") };
		for (int32 i = ShariNext; i < ShariBeats; ++i)
		{
			const float NoteX = HitX + static_cast<float>((ShariBeatTime(i) - Now) * PixelsPerSecond);
			if (NoteX > X + W - 30.f)
			{
				break;
			}
			HUD->DrawRect(FLinearColor(0.97f, 0.97f, 0.92f, 1.f), NoteX - 14.f, Y + 10.f, 28.f, LaneH - 20.f);
			HUD->DrawLabel(BeatNames[i], NoteX - 16.f, Y + LaneH + 6.f, FLinearColor(1.f, 0.95f, 0.8f), 14);
		}
		return;
	}

	if (Phase == ESushiCookPhase::Wasabi)
	{
		HUD->DrawLabel(LOCTEXT("WasabiTitle", "와사비 룰렛 — 초록 구간(10~35%)이 적정!"), X, Y - 62.f, FLinearColor::White, 20);
		const ASushiCustomer* Customer = Target.Get();
		if (Customer && Customer->GetKind() == ESushiCustomerKind::Thrill)
		{
			HUD->DrawLabel(LOCTEXT("ThrillWish", "날다람G: 듬뿍! 85% 넘게 넣어 줘유! 날아가고 싶어유!"), X, Y - 36.f, FLinearColor(1.f, 0.7f, 0.4f), 15);
		}
		else if (bRussianWasabi)
		{
			HUD->DrawLabel(LOCTEXT("RussianHint", "…와사비 통에서 아지랑이가 피어오른다?"), X, Y - 36.f, FLinearColor(0.6f, 1.f, 0.5f), 15);
		}

		const float BarH = 40.f;
		struct FZone { float From; float To; FLinearColor Color; };
		const FZone Zones[] = {
			{ 0.f, 0.1f, FLinearColor(0.5f, 0.5f, 0.5f) },
			{ 0.1f, 0.35f, FLinearColor(0.3f, 0.85f, 0.3f) },
			{ 0.35f, 0.6f, FLinearColor(0.95f, 0.9f, 0.3f) },
			{ 0.6f, 0.85f, FLinearColor(1.f, 0.55f, 0.15f) },
			{ 0.85f, 1.f, FLinearColor(1.f, 0.15f, 0.15f) },
		};
		for (const FZone& Zone : Zones)
		{
			HUD->DrawRect(Zone.Color, X + Zone.From * W, Y, (Zone.To - Zone.From) * W, BarH);
		}
		HUD->DrawLabel(LOCTEXT("ZoneRocket", "로켓"), X + 0.88f * W, Y + BarH + 4.f, FLinearColor(1.f, 0.4f, 0.4f), 14);
		HUD->DrawLabel(LOCTEXT("ZoneGood", "적정"), X + 0.18f * W, Y + BarH + 4.f, FLinearColor(0.5f, 1.f, 0.5f), 14);

		const float Needle = CurrentWasabiNeedle(Now);
		HUD->DrawRect(FLinearColor::White, X + Needle * W - 3.f, Y - 10.f, 6.f, BarH + 20.f);
		return;
	}

	if (Phase == ESushiCookPhase::Result)
	{
		HUD->DrawLabel(FText::Format(LOCTEXT("ResultTitle", "{0} 초밥 대접!"), Ingredient.DisplayName), X, Y - 62.f, FLinearColor(1.f, 0.9f, 0.5f), 22);
		HUD->DrawLabel(FText::Format(LOCTEXT("ResultLine", "{0}점   별점 {1}/5   +{2}코인"),
			FText::AsNumber(FMath::RoundToInt(ResultScore)), FText::AsNumber(ResultStars), FText::AsNumber(ResultCoins)), X, Y - 24.f, FLinearColor::White, 26);
		HUD->DrawLabel(LOCTEXT("ResultFormula", "점수 = 재료 35% + 샤리 20% + 와사비 15% + 플레이팅 20% + 취향 10%, × 신선도 보정"),
			X, Y + 24.f, FLinearColor(0.8f, 0.8f, 0.8f), 14);
	}
}

#undef LOCTEXT_NAMESPACE
