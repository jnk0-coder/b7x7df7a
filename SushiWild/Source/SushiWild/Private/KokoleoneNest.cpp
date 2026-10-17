#include "KokoleoneNest.h"

#include "Components/StaticMeshComponent.h"
#include "CornBait.h"
#include "Engine/Canvas.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "SushiCatalog.h"
#include "SushiChefCharacter.h"
#include "SushiHUD.h"
#include "SushiShapes.h"
#include "SushiVisionComponent.h"
#include "SushiWildGameState.h"

#define LOCTEXT_NAMESPACE "Kokoleone"

namespace
{
	const FLinearColor BossColor(1.f, 0.6f, 0.6f);
}

AKokoleoneNest::AKokoleoneNest()
{
	PrimaryActorTick.bCanEverTick = true;
	InteractRadius = 330.f;

	GazePivot = CreateDefaultSubobject<USceneComponent>(TEXT("GazePivot"));
	GazePivot->SetupAttachment(RootComponent);

	Vision = CreateDefaultSubobject<USushiVisionComponent>(TEXT("Vision"));
	Vision->SetupAttachment(GazePivot);
	Vision->SetRelativeLocation(FVector(110.f, 0.f, 210.f));
	Vision->Range = 1500.f;
	Vision->HalfAngleDeg = 35.f;
	Vision->HearingRange = 900.f;
}

void AKokoleoneNest::BeginPlay()
{
	Super::BeginPlay();
	BuildVisuals();
}

void AKokoleoneNest::BuildVisuals()
{
	const FLinearColor Feather(0.98f, 0.95f, 0.88f);
	const FLinearColor Beak(1.f, 0.6f, 0.1f);
	const FLinearColor Black(0.03f, 0.03f, 0.03f);

	// 둥지 (부딪힘 있음)
	SushiShapes::Add(this, nullptr, ESushiShape::Cylinder, FVector(0.f, 0.f, 17.f), FVector(1.7f, 1.7f, 0.35f), FLinearColor(0.45f, 0.3f, 0.15f), true);

	// 대부 엉덩이 밑으로 삐져나온 알 (뒤쪽 = -X)
	EggMesh = SushiShapes::Add(this, nullptr, ESushiShape::Sphere, FVector(-75.f, 30.f, 48.f), FVector(0.3f, 0.3f, 0.38f), FLinearColor(0.97f, 0.95f, 0.88f));

	// 거대 암탉 몸통 + 날개
	SushiShapes::Add(this, GazePivot, ESushiShape::Sphere, FVector(0.f, 0.f, 120.f), FVector(1.8f, 1.6f, 1.5f), Feather, true);
	SushiShapes::Add(this, GazePivot, ESushiShape::Sphere, FVector(-5.f, 85.f, 120.f), FVector(0.9f, 0.3f, 0.6f), Feather);
	SushiShapes::Add(this, GazePivot, ESushiShape::Sphere, FVector(-5.f, -85.f, 120.f), FVector(0.9f, 0.3f, 0.6f), Feather);
	SushiShapes::Add(this, GazePivot, ESushiShape::Sphere, FVector(-95.f, 0.f, 160.f), FVector(0.5f, 0.7f, 0.9f), Feather);

	// 머리, 부리, 볏
	SushiShapes::Add(this, GazePivot, ESushiShape::Sphere, FVector(80.f, 0.f, 205.f), FVector(0.95f), Feather);
	SushiShapes::Add(this, GazePivot, ESushiShape::Cone, FVector(130.f, 0.f, 198.f), FVector(0.3f, 0.3f, 0.35f), Beak, false, FRotator(-90.f, 0.f, 0.f));
	for (int32 i = 0; i < 3; ++i)
	{
		SushiShapes::Add(this, GazePivot, ESushiShape::Sphere, FVector(60.f + i * 18.f, 0.f, 250.f + (i == 1 ? 8.f : 0.f)), FVector(0.22f), FLinearColor(0.9f, 0.1f, 0.1f));
	}

	// 굵은 눈썹 + 눈 (화난 표정)
	for (const float Side : { -1.f, 1.f })
	{
		SushiShapes::Add(this, GazePivot, ESushiShape::Sphere, FVector(118.f, 22.f * Side, 215.f), FVector(0.12f), Black);
		SushiShapes::Add(this, GazePivot, ESushiShape::Cube, FVector(120.f, 22.f * Side, 232.f), FVector(0.05f, 0.28f, 0.07f), Black, false, FRotator(0.f, 0.f, 18.f * Side));
	}

	// 마피아 중절모 + 시가 대신 문 지렁이
	SushiShapes::Add(this, GazePivot, ESushiShape::Cylinder, FVector(75.f, 0.f, 262.f), FVector(0.85f, 0.85f, 0.06f), Black);
	SushiShapes::Add(this, GazePivot, ESushiShape::Cylinder, FVector(75.f, 0.f, 282.f), FVector(0.55f, 0.55f, 0.35f), Black);
	SushiShapes::Add(this, GazePivot, ESushiShape::Cylinder, FVector(150.f, 18.f, 188.f), FVector(0.07f, 0.07f, 0.45f), FLinearColor(1.f, 0.55f, 0.6f), false, FRotator(-75.f, 15.f, 0.f));
}

void AKokoleoneNest::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	ASushiWildGameState* State = GetWorld()->GetGameState<ASushiWildGameState>();
	ASushiChefCharacter* Chef = Cast<ASushiChefCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));

	UpdateGaze(DeltaSeconds, State, Chef);
	UpdateSuspicion(DeltaSeconds, State, Chef);
	CheckCornTaunt(State);

	if (bPulling)
	{
		PullProgress += DeltaSeconds / FMath::Max(PullSeconds, 0.1f);
		if (PullProgress >= 1.f)
		{
			CompleteHeist();
		}
	}
	else
	{
		PullProgress = FMath::Max(0.f, PullProgress - DeltaSeconds * 0.2f);
	}

	if (!bHasEgg)
	{
		EggRespawnLeft -= DeltaSeconds;
		if (EggRespawnLeft <= 0.f)
		{
			bHasEgg = true; // "내일 또 와라, 이 요리사 놈아!"
			EggMesh->SetVisibility(true);
		}
	}
}

void AKokoleoneNest::UpdateGaze(float DeltaSeconds, ASushiWildGameState* State, const ASushiChefCharacter* Chef)
{
	float YawOffset;
	if (bFakeLookBack)
	{
		FakeLookBackLeft -= DeltaSeconds;
		YawOffset = 180.f;
		bFakeLookBack = FakeLookBackLeft > 0.f;
	}
	else
	{
		SweepTime += DeltaSeconds;
		YawOffset = SweepAmplitudeDeg * FMath::Sin(2.f * PI * SweepTime / SweepPeriodSeconds);

		const int32 Cycle = FMath::FloorToInt(SweepTime / SweepPeriodSeconds);
		if (Cycle != LastSweepCycle)
		{
			LastSweepCycle = Cycle;
			if (State && State->GetRng().FRand() < FakeLookBackChance)
			{
				bFakeLookBack = true;
				FakeLookBackLeft = FakeLookBackSeconds;
				if (Chef && FVector::Dist2D(Chef->GetActorLocation(), GetActorLocation()) < 1800.f)
				{
					State->PushToast(LOCTEXT("FakeLook", "꼬꼬레오네: …흠칫! (뒤돌아보는 척)"), BossColor, 2.f);
				}
			}
		}
	}
	GazePivot->SetRelativeRotation(FRotator(0.f, YawOffset, 0.f));
}

void AKokoleoneNest::UpdateSuspicion(float DeltaSeconds, ASushiWildGameState* State, ASushiChefCharacter* Chef)
{
	const float RangeMult = State ? State->GetVisionRangeMultiplier() : 1.f;
	const float NoiseMult = State ? State->GetNoiseMultiplier() : 1.f;
	Vision->SetRangeMultiplier(RangeMult);

	if (IsAlarmed())
	{
		AlarmTimeLeft -= DeltaSeconds;
		Vision->SetAlertLevel(1.f);
		if (!IsAlarmed())
		{
			Suspicion = 0.3f;
			if (State)
			{
				State->PushToast(LOCTEXT("AlarmOver", "꼬꼬레오네: …흥, 쥐새끼였나. 다들 제자리로!"), BossColor);
			}
		}
		return;
	}

	const float Sight = Vision->ComputeSight(Chef, RangeMult);
	const float Hearing = Vision->ComputeHearing(Chef, NoiseMult);
	float Gain = Sight * 1.2f + Hearing * 0.8f;
	if (bPulling)
	{
		// 엉덩이 밑에서 부스럭거리면 조금씩 수상해지고, 보이면 순식간에 들킨다.
		Gain += 0.12f + (Sight > 0.f ? 2.f : 0.f);
	}

	Suspicion = Gain > 0.f ? Suspicion + Gain * DeltaSeconds : Suspicion - 0.2f * DeltaSeconds;
	Suspicion = FMath::Clamp(Suspicion, 0.f, 1.f);

	if (State && Suspicion >= 0.35f && !bSaidSuspicious)
	{
		bSaidSuspicious = true;
		State->PushToast(LOCTEXT("Suspicious", "꼬꼬레오네: …뭐여? 거기 누구여?"), BossColor, 2.5f);
	}
	if (Suspicion < 0.1f)
	{
		bSaidSuspicious = false;
	}

	if (Suspicion >= 1.f)
	{
		TriggerAlarm(State);
	}
	Vision->SetAlertLevel(Suspicion * 0.9f);
}

void AKokoleoneNest::ReportSighting(float Amount)
{
	if (!IsAlarmed())
	{
		Suspicion = FMath::Clamp(Suspicion + Amount, 0.f, 1.f);
	}
}

void AKokoleoneNest::CheckCornTaunt(ASushiWildGameState* State)
{
	if (!State)
	{
		return;
	}
	for (TActorIterator<ACornBait> It(GetWorld()); It; ++It)
	{
		if (It->HasLanded() && LastTauntedCorn.Get() != *It && FVector::Dist2D(It->GetActorLocation(), GetActorLocation()) < 700.f)
		{
			LastTauntedCorn = *It;
			State->PushToast(LOCTEXT("CornTaunt", "꼬꼬레오네: 옥수수? 날 병아리로 아나?"), BossColor, 2.5f);
			return;
		}
	}
}

void AKokoleoneNest::TriggerAlarm(ASushiWildGameState* State)
{
	AlarmTimeLeft = AlarmSeconds;
	Suspicion = 1.f;
	if (bPulling)
	{
		StopPulling(true);
	}
	if (!State)
	{
		return;
	}
	++State->FarmAlarmCount;
	State->PushToast(LOCTEXT("Alarm", "꼬끼오오오오! 농장 전체 경보! 30초 버티거나 멀리 도망쳐유!"), FLinearColor(1.f, 0.2f, 0.2f), 4.f);
	if (State->FarmAlarmCount % 3 == 0)
	{
		State->PushToast(LOCTEXT("Wanted", "농장 입구에 내 얼굴 현상수배 포스터가 붙었슈. (현상금: 지렁이 100마리)"), FLinearColor(1.f, 0.6f, 0.3f), 5.f);
	}
}

bool AKokoleoneNest::CanInteract(const ASushiChefCharacter* Chef) const
{
	return bHasEgg && !IsAlarmed() && Chef && !Chef->IsCarryingEgg();
}

FText AKokoleoneNest::GetPrompt(const ASushiChefCharacter* Chef) const
{
	return bPulling
		? LOCTEXT("PromptPulling", "알 빼내는 중… 손 떼지 마유!")
		: LOCTEXT("Prompt", "[E 꾹 누르기] 대부 엉덩이 밑에서 알 빼내기");
}

void AKokoleoneNest::OnInteractPressed(ASushiChefCharacter* Chef)
{
	if (!CanInteract(Chef) || bPulling)
	{
		return;
	}
	bPulling = true;
	Puller = Chef;
	Chef->LockToMinigame(this);
}

void AKokoleoneNest::OnInteractReleased(ASushiChefCharacter* Chef)
{
	if (bPulling)
	{
		StopPulling(false);
	}
}

void AKokoleoneNest::StopPulling(bool bFailed)
{
	bPulling = false;
	if (ASushiChefCharacter* Chef = Puller.Get())
	{
		Chef->UnlockMinigame(this);
	}
	Puller.Reset();
	if (bFailed)
	{
		PullProgress = 0.f;
		if (ASushiWildGameState* State = GetWorld()->GetGameState<ASushiWildGameState>())
		{
			State->RegisterMissionResult(SushiMissions::EggHeist, false);
		}
	}
}

void AKokoleoneNest::CompleteHeist()
{
	ASushiChefCharacter* Chef = Puller.Get();
	ASushiWildGameState* State = GetWorld()->GetGameState<ASushiWildGameState>();
	const bool bUndetected = Suspicion < 0.35f;
	StopPulling(false);
	PullProgress = 0.f;
	if (!Chef || !State)
	{
		return;
	}

	bHasEgg = false;
	EggRespawnLeft = EggRespawnSeconds;
	EggMesh->SetVisibility(false);

	const ESushiRarity Rarity = State->RollRarity();
	const float Quality = FMath::Clamp(60.f + (bUndetected ? SushiCatalog::CatchStyleQualityBonus(ESushiCatchStyle::Stealth) : 0.f)
		+ State->GetRng().FRandRange(0.f, 15.f), 0.f, 100.f);
	const FSushiIngredient Egg = SushiCatalog::MakeEgg(Rarity, Quality, GetWorld()->GetTimeSeconds());
	Chef->StartCarryingEgg(Egg);

	State->PushToast(FText::Format(LOCTEXT("GotEgg", "알 획득! [{0}] {1} — 이제 살금살금 가게로. 뛰면 깨져유!"),
		SushiCatalog::RarityName(Rarity), Egg.DisplayName), SushiCatalog::RarityColor(Rarity), 5.f);
	if (Rarity == ESushiRarity::Legend)
	{
		State->PushToast(LOCTEXT("GoldenEgg", "황금알!! 대부가 이걸 알면… 생각만 해도 무섭구먼유."), SushiCatalog::RarityColor(Rarity), 5.f);
	}
}

void AKokoleoneNest::DrawMinigameHUD(ASushiHUD* HUD, UCanvas* Canvas) const
{
	if (!HUD || !Canvas)
	{
		return;
	}
	const float W = 420.f;
	const float X = (Canvas->ClipX - W) * 0.5f;
	const float Y = Canvas->ClipY * 0.62f;
	HUD->DrawPanel(X - 16.f, Y - 44.f, W + 32.f, 110.f);
	HUD->DrawLabel(LOCTEXT("PullTitle", "알 빼내기 — 대부 시선(바닥 부채꼴)에 안 걸리게!"), X, Y - 36.f, FLinearColor::White, 18);
	HUD->DrawBar(X, Y, W, 22.f, PullProgress, FLinearColor(1.f, 0.95f, 0.8f), LOCTEXT("PullBar", "알"));
	HUD->DrawBar(X, Y + 34.f, W, 12.f, Suspicion, FLinearColor(1.f, 0.35f, 0.2f), LOCTEXT("SuspicionBar", "대부 의심"));
}

#undef LOCTEXT_NAMESPACE
