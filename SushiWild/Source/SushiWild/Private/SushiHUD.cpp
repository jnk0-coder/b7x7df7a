#include "SushiHUD.h"

#include "CanvasItem.h"
#include "Engine/Canvas.h"
#include "Engine/Font.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Styling/CoreStyle.h"
#include "SushiCatalog.h"
#include "SushiChefCharacter.h"
#include "SushiCustomer.h"
#include "SushiInteractable.h"
#include "SushiWildGameState.h"
#include "SushiWorldLayout.h"

#define LOCTEXT_NAMESPACE "SushiHUD"

void ASushiHUD::DrawHUD()
{
	Super::DrawHUD();
	if (!Canvas)
	{
		return;
	}
	DrawWorldLabels();
	DrawCustomers();
	DrawStatus();
	DrawChefBars();
	DrawInventory();
	DrawPromptAndMinigame();
	DrawToasts();
	DrawControls();
}

void ASushiHUD::DrawLabel(const FText& Text, float X, float Y, const FLinearColor& Color, int32 Size, bool bCenterX)
{
	if (!Canvas)
	{
		return;
	}
	const FSlateFontInfo Font = KoreanFontOverride
		? FSlateFontInfo(KoreanFontOverride, Size)
		: FCoreStyle::GetDefaultFontStyle("Bold", Size);
	FCanvasTextItem Item(FVector2D(X, Y), Text, Font, Color);
	Item.EnableShadow(FLinearColor(0.f, 0.f, 0.f, 0.8f));
	Item.bCentreX = bCenterX;
	Canvas->DrawItem(Item);
}

void ASushiHUD::DrawBar(float X, float Y, float W, float H, float Fill01, const FLinearColor& Color, const FText& Label)
{
	DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.6f), X - 2.f, Y - 2.f, W + 4.f, H + 4.f);
	DrawRect(Color, X, Y, W * FMath::Clamp(Fill01, 0.f, 1.f), H);
	if (!Label.IsEmpty())
	{
		DrawLabel(Label, X + 6.f, Y + H * 0.5f - 9.f, FLinearColor::White, 13);
	}
}

void ASushiHUD::DrawPanel(float X, float Y, float W, float H, float Alpha)
{
	DrawRect(FLinearColor(0.02f, 0.03f, 0.06f, Alpha), X, Y, W, H);
}

bool ASushiHUD::ProjectToScreen(const FVector& World, FVector2D& OutScreen) const
{
	const FVector Screen = Project(World, false);
	if (Screen.Z <= 0.f)
	{
		return false; // 카메라 뒤
	}
	OutScreen = FVector2D(Screen.X, Screen.Y);
	return OutScreen.X >= 0.f && OutScreen.Y >= 0.f && OutScreen.X <= Canvas->ClipX && OutScreen.Y <= Canvas->ClipY;
}

void ASushiHUD::DrawStatus()
{
	const ASushiWildGameState* State = GetWorld()->GetGameState<ASushiWildGameState>();
	if (!State)
	{
		return;
	}
	DrawPanel(12.f, 12.f, 380.f, 132.f);
	DrawLabel(State->GetClockText(), 24.f, 20.f, FLinearColor::White, 22);
	DrawLabel(FText::Format(LOCTEXT("Weather", "날씨: {0}"), State->GetWeatherText()), 24.f, 52.f, FLinearColor(0.7f, 0.9f, 1.f), 16);
	DrawLabel(FText::Format(LOCTEXT("Forecast", "개굴레오 예보: 오늘 밤 비 {0}%"),
		FText::AsNumber(FMath::RoundToInt(State->GetTonightRainChance() * 100.f))), 24.f, 76.f, FLinearColor(0.6f, 1.f, 0.6f), 15);
	DrawLabel(FText::Format(LOCTEXT("Shop", "간판 \"{0}\"  ·  별점 {1}  ·  {2}코인"),
		State->GetSignText(),
		FText::AsNumber(FMath::RoundToInt(State->StarRating * 10.f) / 10.f),
		FText::AsNumber(State->Coins)), 24.f, 104.f, FLinearColor(1.f, 0.85f, 0.4f), 17);
}

void ASushiHUD::DrawChefBars()
{
	const ASushiChefCharacter* Chef = Cast<ASushiChefCharacter>(GetOwningPawn());
	if (!Chef)
	{
		return;
	}
	const float X = 24.f;
	const float Y = 160.f;
	DrawPanel(12.f, Y - 8.f, 260.f, Chef->IsCarryingEgg() ? 132.f : 104.f);
	DrawBar(X, Y, 230.f, 18.f, Chef->GetHealth() / Chef->MaxHealth, FLinearColor(0.95f, 0.3f, 0.3f), LOCTEXT("Health", "밥공기(체력)"));
	DrawBar(X, Y + 26.f, 230.f, 14.f, Chef->GetStamina() / 100.f, FLinearColor(0.35f, 0.75f, 1.f), LOCTEXT("Stamina", "간장 종지(스태미나)"));
	DrawBar(X, Y + 48.f, 230.f, 14.f, Chef->GetNoise(), FLinearColor(1.f, 0.85f, 0.3f), LOCTEXT("Noise", "발소리"));
	DrawLabel(FText::Format(LOCTEXT("Corn", "옥수수 미끼 x{0}"), FText::AsNumber(Chef->GetCornCount())), X, Y + 70.f, FLinearColor(1.f, 0.9f, 0.4f), 14);
	if (Chef->IsCarryingEgg())
	{
		DrawBar(X, Y + 96.f, 230.f, 16.f, Chef->GetEggWobble(), FLinearColor(1.f, 0.55f, 0.2f), LOCTEXT("Wobble", "알 흔들림 (뛰면 깨짐!)"));
	}
}

void ASushiHUD::DrawInventory()
{
	const ASushiChefCharacter* Chef = Cast<ASushiChefCharacter>(GetOwningPawn());
	if (!Chef)
	{
		return;
	}
	const TArray<FSushiIngredient>& Items = Chef->GetInventory();
	const float W = 360.f;
	const float X = Canvas->ClipX - W - 12.f;
	const float Y = 12.f;
	DrawPanel(X, Y, W, 40.f + 26.f * FMath::Max(1, Items.Num()));
	DrawLabel(FText::Format(LOCTEXT("Bag", "가방 {0}/{1}"), FText::AsNumber(Items.Num()), FText::AsNumber(Chef->BagSlots)), X + 12.f, Y + 8.f, FLinearColor::White, 17);
	if (Items.Num() == 0)
	{
		DrawLabel(LOCTEXT("BagEmpty", "텅 비었슈"), X + 12.f, Y + 36.f, FLinearColor(0.7f, 0.7f, 0.7f), 15);
		return;
	}
	for (int32 i = 0; i < Items.Num(); ++i)
	{
		const FSushiIngredient& Item = Items[i];
		const float Fresh = Chef->GetFreshness(Item);
		const FLinearColor FreshColor = Fresh > 60.f ? FLinearColor(0.5f, 1.f, 0.5f) : (Fresh > 25.f ? FLinearColor(1.f, 0.85f, 0.3f) : FLinearColor(1.f, 0.4f, 0.3f));
		DrawLabel(FText::Format(LOCTEXT("Item", "[{0}] {1}"), SushiCatalog::RarityName(Item.Rarity), Item.DisplayName),
			X + 12.f, Y + 36.f + 26.f * i, SushiCatalog::RarityColor(Item.Rarity), 14);
		DrawLabel(FText::Format(LOCTEXT("Fresh", "{0}%"), FText::AsNumber(FMath::RoundToInt(Fresh))), X + W - 56.f, Y + 36.f + 26.f * i, FreshColor, 14);
	}
}

void ASushiHUD::DrawToasts()
{
	const ASushiWildGameState* State = GetWorld()->GetGameState<ASushiWildGameState>();
	if (!State)
	{
		return;
	}
	float Y = Canvas->ClipY * 0.16f;
	for (const FSushiToast& Toast : State->GetToasts())
	{
		FLinearColor Color = Toast.Color;
		Color.A = FMath::Clamp(Toast.TimeLeft / 0.5f, 0.f, 1.f);
		DrawLabel(Toast.Text, Canvas->ClipX * 0.5f, Y, Color, 20, true);
		Y += 30.f;
	}
}

void ASushiHUD::DrawWorldLabels()
{
	struct FLabel { FVector Location; FText Text; };
	const ASushiWildGameState* State = GetWorld()->GetGameState<ASushiWildGameState>();
	const FText Sign = State ? State->GetSignText() : FText::GetEmpty();
	const FLabel Labels[] = {
		{ SushiLayout::Counter + FVector(-50.f, 0.f, 330.f), FText::Format(LOCTEXT("SignLabel", "간판: {0}"), Sign) },
		{ SushiLayout::Farm + FVector(0.f, 0.f, 450.f), LOCTEXT("Farm", "꼬꼬댁 농장 (마피아 소굴)") },
		{ SushiLayout::Beach + FVector(0.f, 0.f, 300.f), LOCTEXT("Beach", "고등어 해변") },
		{ SushiLayout::FishingSpot + FVector(0.f, 0.f, 260.f), LOCTEXT("FishingSpot", "리듬 낚시터") },
		{ SushiLayout::Forest + FVector(0.f, 0.f, 600.f), LOCTEXT("Forest", "고추냉이 숲 (멧돼지 추격전은 v0.2)") },
	};
	for (const FLabel& Label : Labels)
	{
		FVector2D Screen;
		if (ProjectToScreen(Label.Location, Screen))
		{
			DrawLabel(Label.Text, Screen.X, Screen.Y, FLinearColor(1.f, 1.f, 0.9f), 18, true);
		}
	}
}

void ASushiHUD::DrawCustomers()
{
	const APawn* Pawn = GetOwningPawn();
	for (TActorIterator<ASushiCustomer> It(GetWorld()); It; ++It)
	{
		const ASushiCustomer* Customer = *It;
		FVector2D Screen;
		if (Customer->IsWaiting() && ProjectToScreen(Customer->GetActorLocation() + FVector(0.f, 0.f, 230.f), Screen))
		{
			const float P = Customer->GetPatience01();
			DrawBar(Screen.X - 50.f, Screen.Y, 100.f, 8.f, P, FMath::Lerp(FLinearColor(1.f, 0.2f, 0.2f), FLinearColor(0.4f, 1.f, 0.4f), P), FText::GetEmpty());
		}
		else if (Customer->IsCrashed() && Pawn)
		{
			// 추락 손님 표시: 화면 안이면 머리 위에, 밖이면 거리만
			const float Meters = FVector::Dist2D(Pawn->GetActorLocation(), Customer->GetActorLocation()) / 100.f;
			const FText Text = FText::Format(LOCTEXT("Crashed", "추락 손님 {0}m"), FText::AsNumber(FMath::RoundToInt(Meters)));
			if (ProjectToScreen(Customer->GetActorLocation() + FVector(0.f, 0.f, 150.f), Screen))
			{
				DrawLabel(Text, Screen.X, Screen.Y, FLinearColor(1.f, 0.6f, 0.2f), 18, true);
			}
			else
			{
				const FVector To = Customer->GetActorLocation() - Pawn->GetActorLocation();
				const float Bearing = FMath::RadiansToDegrees(FMath::Atan2(To.Y, To.X));
				const FText Dir = FMath::Abs(Bearing) < 45.f ? LOCTEXT("East", "동쪽")
					: (FMath::Abs(Bearing) > 135.f ? LOCTEXT("West", "서쪽") : (Bearing > 0.f ? LOCTEXT("North", "북쪽") : LOCTEXT("South", "남쪽")));
				DrawLabel(FText::Format(LOCTEXT("CrashedFar", "{0} ({1})"), Text, Dir), Canvas->ClipX * 0.5f, Canvas->ClipY - 150.f, FLinearColor(1.f, 0.6f, 0.2f), 16, true);
			}
		}
	}
}

void ASushiHUD::DrawPromptAndMinigame()
{
	const ASushiChefCharacter* Chef = Cast<ASushiChefCharacter>(GetOwningPawn());
	if (!Chef)
	{
		return;
	}
	if (ASushiInteractable* Game = Chef->GetMinigameOwner())
	{
		Game->DrawMinigameHUD(this, Canvas);
	}
	if (const ASushiInteractable* Focus = Chef->GetFocusedInteractable())
	{
		const FText Prompt = Focus->GetPrompt(Chef);
		if (!Prompt.IsEmpty())
		{
			DrawPanel(Canvas->ClipX * 0.5f - 330.f, Canvas->ClipY - 104.f, 660.f, 40.f);
			DrawLabel(Prompt, Canvas->ClipX * 0.5f, Canvas->ClipY - 96.f, FLinearColor::White, 20, true);
		}
	}
}

void ASushiHUD::DrawControls()
{
	DrawLabel(LOCTEXT("Controls", "WASD 이동 · Shift 달리기 · Ctrl/C 웅크리기 · Space 점프 · E/좌클릭 상호작용 · Q 옥수수 던지기"),
		12.f, Canvas->ClipY - 30.f, FLinearColor(0.85f, 0.85f, 0.85f, 0.8f), 14);
}

#undef LOCTEXT_NAMESPACE
