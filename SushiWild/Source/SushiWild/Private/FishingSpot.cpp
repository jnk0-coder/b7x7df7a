#include "FishingSpot.h"

#include "Engine/Canvas.h"
#include "SushiBalance.h"
#include "SushiCatalog.h"
#include "SushiChefCharacter.h"
#include "SushiHUD.h"
#include "SushiShapes.h"
#include "SushiWildGameState.h"
#include "SushiWorldLayout.h"

#define LOCTEXT_NAMESPACE "Fishing"

namespace
{
	constexpr double EarlyIgnoreSeconds = 0.25; // 이보다 일찍 누른 건 무시 (대신 헛손질 벌점)
}

AFishingSpot::AFishingSpot()
{
	PrimaryActorTick.bCanEverTick = true;
	InteractRadius = 260.f;
}

void AFishingSpot::BeginPlay()
{
	Super::BeginPlay();
	// 낚싯대 거치대 + 양동이 + 안내 깃발
	SushiShapes::Add(this, nullptr, ESushiShape::Cylinder, FVector(60.f, 60.f, 60.f), FVector(0.08f, 0.08f, 1.2f), FLinearColor(0.4f, 0.25f, 0.1f));
	SushiShapes::Add(this, nullptr, ESushiShape::Cylinder, FVector(140.f, 60.f, 130.f), FVector(0.03f, 0.03f, 2.f), FLinearColor(0.2f, 0.2f, 0.2f), false, FRotator(-60.f, 0.f, 0.f));
	SushiShapes::Add(this, nullptr, ESushiShape::Cylinder, FVector(-40.f, -70.f, 25.f), FVector(0.4f, 0.4f, 0.5f), FLinearColor(0.2f, 0.45f, 0.85f));
	SushiShapes::Add(this, nullptr, ESushiShape::Cube, FVector(0.f, -120.f, 160.f), FVector(0.05f, 0.6f, 0.4f), FLinearColor(1.f, 0.3f, 0.3f));
}

FText AFishingSpot::GetPrompt(const ASushiChefCharacter* Chef) const
{
	return bActive ? LOCTEXT("PromptActive", "박자에 맞춰 [E]!") : LOCTEXT("Prompt", "[E] 리듬 낚시 시작");
}

void AFishingSpot::OnInteractPressed(ASushiChefCharacter* Chef)
{
	if (!bActive)
	{
		StartFishing(Chef);
		return;
	}
	Judge(GetWorld()->GetTimeSeconds());
}

void AFishingSpot::StartFishing(ASushiChefCharacter* Chef)
{
	ASushiWildGameState* State = GetWorld()->GetGameState<ASushiWildGameState>();
	if (!Chef || !State)
	{
		return;
	}

	FishIndex = State->GetRng().RandRange(0, SushiCatalog::NumFish() - 1);
	const SushiCatalog::FFishInfo Fish = SushiCatalog::GetFish(FishIndex);
	BeatSeconds = 60.0 / Fish.Bpm;
	StartTime = GetWorld()->GetTimeSeconds() + LeadInBeats * BeatSeconds;
	NextNote = 0;
	Tension = 30.f;
	Perfects = Goods = Misses = 0;
	WindowScale = State->GetJudgeWindowScale(SushiMissions::Fishing);
	LastJudgeTime = -10.0;

	bActive = true;
	Angler = Chef;
	Chef->LockToMinigame(this);

	State->PushToast(FText::Format(LOCTEXT("Bite", "입질! {0} · {1} {2}BPM — 박자에 맞춰 E!"),
		Fish.Name, Fish.Genre, FText::AsNumber(FMath::RoundToInt(Fish.Bpm))), FLinearColor(0.5f, 0.85f, 1.f));
}

void AFishingSpot::Judge(double Now)
{
	if (NextNote >= NoteCount)
	{
		return;
	}
	const double Error = Now - NoteTime(NextNote);
	if (Error < -EarlyIgnoreSeconds)
	{
		Tension += 5.f; // 헛손질
		return;
	}

	switch (SushiBalance::JudgeBeat(Error, WindowScale))
	{
	case SushiBalance::Perfect:
		++Perfects;
		Tension -= 10.f;
		LastJudgeText = LOCTEXT("Perfect", "Perfect!");
		LastJudgeColor = FLinearColor(1.f, 0.9f, 0.3f);
		break;
	case SushiBalance::Good:
		++Goods;
		Tension -= 5.f;
		LastJudgeText = LOCTEXT("Good", "Good");
		LastJudgeColor = FLinearColor(0.6f, 1.f, 0.6f);
		break;
	default:
		++Misses;
		Tension += 25.f;
		LastJudgeText = LOCTEXT("Miss", "Miss…");
		LastJudgeColor = FLinearColor(1.f, 0.4f, 0.4f);
		break;
	}
	Tension = FMath::Max(0.f, Tension);
	LastJudgeTime = Now;
	++NextNote;
}

void AFishingSpot::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!bActive)
	{
		return;
	}

	const double Now = GetWorld()->GetTimeSeconds();
	// 지나가 버린 노트는 자동 Miss
	while (NextNote < NoteCount && Now - NoteTime(NextNote) > SushiBalance::GoodWindowSec * WindowScale)
	{
		++Misses;
		Tension += 25.f;
		LastJudgeText = LOCTEXT("MissLate", "Miss…");
		LastJudgeColor = FLinearColor(1.f, 0.4f, 0.4f);
		LastJudgeTime = Now;
		++NextNote;
	}

	if (Tension >= 100.f)
	{
		Finish(false);
	}
	else if (NextNote >= NoteCount)
	{
		Finish(true);
	}
}

void AFishingSpot::Finish(bool bCaught)
{
	bActive = false;
	ASushiChefCharacter* Chef = Angler.Get();
	Angler.Reset();
	ASushiWildGameState* State = GetWorld()->GetGameState<ASushiWildGameState>();
	if (!Chef)
	{
		return;
	}
	Chef->UnlockMinigame(this);
	if (State)
	{
		State->RegisterMissionResult(SushiMissions::Fishing, bCaught);
	}

	if (!bCaught)
	{
		// 줄이 끊기기 직전 수상스키로 끌려가다… 해변에 처박힘
		if (State)
		{
			State->PushToast(LOCTEXT("Snap", "투둑! 줄이 끊겼슈. 수상스키처럼 끌려가다 해변에 처박혔슈."), FLinearColor(1.f, 0.5f, 0.3f), 4.f);
		}
		Chef->TeleportTo(SushiLayout::BeachShore, FRotator::ZeroRotator);
		return;
	}

	const float Accuracy = (Perfects + 0.5f * Goods) / static_cast<float>(FMath::Max(NoteCount, 1));
	const float Quality = FMath::Clamp(50.f + 40.f * Accuracy + SushiCatalog::CatchStyleQualityBonus(ESushiCatchStyle::Rhythm), 0.f, 100.f);
	const FSushiIngredient Fish = SushiCatalog::MakeFish(FishIndex, Quality, GetWorld()->GetTimeSeconds());
	if (Chef->AddIngredient(Fish) && State)
	{
		State->PushToast(FText::Format(LOCTEXT("Caught", "{0} 낚았슈! 품질 {1}. 생선은 금방 상하니께 얼른 가게로!"),
			Fish.DisplayName, FText::AsNumber(FMath::RoundToInt(Quality))), FLinearColor(0.5f, 1.f, 0.8f), 4.f);
	}
}

void AFishingSpot::DrawMinigameHUD(ASushiHUD* HUD, UCanvas* Canvas) const
{
	if (!HUD || !Canvas || !bActive)
	{
		return;
	}
	const double Now = GetWorld()->GetTimeSeconds();
	const SushiCatalog::FFishInfo Fish = SushiCatalog::GetFish(FishIndex);

	const float LaneW = 640.f;
	const float LaneH = 60.f;
	const float X = (Canvas->ClipX - LaneW) * 0.5f;
	const float Y = Canvas->ClipY * 0.6f;
	const float HitX = X + 60.f;
	const float PixelsPerSecond = 320.f;

	HUD->DrawPanel(X - 16.f, Y - 70.f, LaneW + 32.f, LaneH + 130.f);
	HUD->DrawLabel(FText::Format(LOCTEXT("Title", "{0} · {1} {2}BPM"), Fish.Name, Fish.Genre, FText::AsNumber(FMath::RoundToInt(Fish.Bpm))),
		X, Y - 62.f, FLinearColor(0.6f, 0.9f, 1.f), 20);

	// 레인 + 판정선
	HUD->DrawRect(FLinearColor(0.f, 0.1f, 0.25f, 0.8f), X, Y, LaneW, LaneH);
	HUD->DrawRect(FLinearColor(1.f, 1.f, 1.f, 0.9f), HitX - 3.f, Y - 6.f, 6.f, LaneH + 12.f);

	for (int32 i = NextNote; i < NoteCount; ++i)
	{
		const float NoteX = HitX + static_cast<float>((NoteTime(i) - Now) * PixelsPerSecond);
		if (NoteX > X + LaneW - 20.f)
		{
			break;
		}
		HUD->DrawRect(FLinearColor(1.f, 0.85f, 0.2f, 1.f), NoteX - 10.f, Y + 10.f, 20.f, LaneH - 20.f);
	}

	if (Now - LastJudgeTime < 0.5)
	{
		HUD->DrawLabel(LastJudgeText, HitX - 20.f, Y - 32.f, LastJudgeColor, 22);
	}
	HUD->DrawBar(X, Y + LaneH + 18.f, LaneW, 14.f, Tension / 100.f, FLinearColor(1.f, 0.3f, 0.2f), LOCTEXT("Tension", "줄 텐션"));
}

#undef LOCTEXT_NAMESPACE
