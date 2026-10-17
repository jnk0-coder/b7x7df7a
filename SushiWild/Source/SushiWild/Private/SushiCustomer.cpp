#include "SushiCustomer.h"

#include "Engine/World.h"
#include "SushiChefCharacter.h"
#include "SushiShapes.h"
#include "SushiWildGameState.h"
#include "SushiWorldLayout.h"

#define LOCTEXT_NAMESPACE "SushiCustomer"

namespace
{
	constexpr float Gravity = 980.f;
	constexpr float StandZ = 0.f; // 액터 원점 = 발바닥
}

ASushiCustomer::ASushiCustomer()
{
	PrimaryActorTick.bCanEverTick = true;
	InteractRadius = 300.f;
}

void ASushiCustomer::InitCustomer(ESushiCustomerKind InKind, FRandomStream& Rng)
{
	Kind = InKind;
	switch (Kind)
	{
	case ESushiCustomerKind::Thrill:
		Taste = FSushiFlavor(1.f, 0.1f, 1.f, 0.3f, 0.4f);
		ShirtColor = FLinearColor(0.55f, 0.35f, 0.2f);
		break;
	case ESushiCustomerKind::Picky:
		Taste = FSushiFlavor(0.2f, 0.6f, 0.1f, 0.9f, 0.3f);
		ShirtColor = FLinearColor(0.15f, 0.15f, 0.2f);
		break;
	default:
		Taste = FSushiFlavor(
			FMath::Clamp(0.3f + Rng.FRandRange(-0.2f, 0.2f), 0.f, 1.f),
			FMath::Clamp(0.5f + Rng.FRandRange(-0.2f, 0.2f), 0.f, 1.f),
			FMath::Clamp(0.3f + Rng.FRandRange(-0.2f, 0.2f), 0.f, 1.f),
			FMath::Clamp(0.5f + Rng.FRandRange(-0.2f, 0.2f), 0.f, 1.f),
			FMath::Clamp(0.5f + Rng.FRandRange(-0.2f, 0.2f), 0.f, 1.f));
		ShirtColor = FLinearColor::MakeFromHSV8(static_cast<uint8>(Rng.RandRange(0, 255)), 160, 220);
		break;
	}
	Patience = MaxPatience;
}

void ASushiCustomer::BeginPlay()
{
	Super::BeginPlay();
	BuildVisuals();
}

void ASushiCustomer::BuildVisuals()
{
	const FLinearColor Skin = Kind == ESushiCustomerKind::Thrill ? FLinearColor(0.6f, 0.38f, 0.2f) : FLinearColor(1.f, 0.82f, 0.68f);
	SushiShapes::Add(this, nullptr, ESushiShape::Cylinder, FVector(0.f, 0.f, 60.f), FVector(0.65f, 0.65f, 1.2f), ShirtColor);
	SushiShapes::Add(this, nullptr, ESushiShape::Sphere, FVector(0.f, 0.f, 150.f), FVector(0.6f), Skin);
	SushiShapes::Add(this, nullptr, ESushiShape::Sphere, FVector(-26.f, -10.f, 157.f), FVector(0.08f), FLinearColor::Black);
	SushiShapes::Add(this, nullptr, ESushiShape::Sphere, FVector(-26.f, 10.f, 157.f), FVector(0.08f), FLinearColor::Black);

	if (Kind == ESushiCustomerKind::Thrill)
	{
		// 비행 고글 + 복슬복슬 꼬리
		SushiShapes::Add(this, nullptr, ESushiShape::Cube, FVector(-24.f, 0.f, 162.f), FVector(0.06f, 0.45f, 0.12f), FLinearColor(0.2f, 0.6f, 1.f));
		SushiShapes::Add(this, nullptr, ESushiShape::Sphere, FVector(45.f, 0.f, 110.f), FVector(0.45f, 0.35f, 1.1f), FLinearColor(0.7f, 0.45f, 0.25f), false, FRotator(-25.f, 0.f, 0.f));
	}
	else if (Kind == ESushiCustomerKind::Picky)
	{
		// 예술가 베레모
		SushiShapes::Add(this, nullptr, ESushiShape::Cylinder, FVector(0.f, 8.f, 185.f), FVector(0.6f, 0.6f, 0.1f), FLinearColor(0.6f, 0.05f, 0.1f), false, FRotator(0.f, 0.f, 12.f));
	}
}

FText ASushiCustomer::GetDisplayName() const
{
	switch (Kind)
	{
	case ESushiCustomerKind::Thrill: return LOCTEXT("Thrill", "날다람G (와사비 듬뿍 원함)");
	case ESushiCustomerKind::Picky:  return LOCTEXT("Picky", "예술가 손님 (비주얼 중시)");
	default:                         return LOCTEXT("Regular", "동네 손님");
	}
}

void ASushiCustomer::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	ASushiWildGameState* State = GetWorld()->GetGameState<ASushiWildGameState>();

	switch (CustomerState)
	{
	case ESushiCustomerState::Waiting:
		Patience -= DeltaSeconds;
		if (Patience <= 0.f)
		{
			if (State)
			{
				State->ApplyReview(1);
				State->PushToast(LOCTEXT("Angry", "손님이 기다리다 지쳐 의자를 뜯어 먹고 갔슈… (별점 테러)"), FLinearColor(1.f, 0.4f, 0.3f), 4.f);
			}
			CustomerState = ESushiCustomerState::Leaving;
			LeaveTimeLeft = 4.f;
		}
		break;

	case ESushiCustomerState::Leaving:
		AddActorWorldOffset(FVector(300.f * DeltaSeconds, 0.f, 0.f));
		LeaveTimeLeft -= DeltaSeconds;
		if (LeaveTimeLeft <= 0.f)
		{
			Destroy();
		}
		break;

	case ESushiCustomerState::Flying:
		TickFlight(DeltaSeconds);
		break;

	default:
		break;
	}
}

void ASushiCustomer::LeaveHappy()
{
	CustomerState = ESushiCustomerState::Leaving;
	LeaveTimeLeft = 4.f;
}

void ASushiCustomer::LaunchRocket(FRandomStream& Rng)
{
	CustomerState = ESushiCustomerState::Flying;
	const float Heading = Rng.FRandRange(0.f, 2.f * PI);
	Velocity = FVector(FMath::Cos(Heading) * 1500.f, FMath::Sin(Heading) * 1500.f, 2500.f);
}

void ASushiCustomer::TickFlight(float DeltaSeconds)
{
	Velocity.Z -= Gravity * DeltaSeconds;
	FVector Next = GetActorLocation() + Velocity * DeltaSeconds;
	const double Limit = SushiLayout::WorldHalfSize - 500.0;
	Next.X = FMath::Clamp(Next.X, -Limit, Limit);
	Next.Y = FMath::Clamp(Next.Y, -Limit, Limit);
	SetActorLocation(Next);
	AddActorWorldRotation(FRotator(0.f, 720.f * DeltaSeconds, 360.f * DeltaSeconds));

	if (Velocity.Z >= 0.f || Next.Z > StandZ)
	{
		return;
	}

	// 착지!
	FVector Landing(Next.X, Next.Y, StandZ);
	FText Where = LOCTEXT("Landed", "손님 착지! 화면의 '추락 손님' 표시를 따라가서 구출해유.");
	if (Landing.X > SushiLayout::ShoreX)
	{
		Landing = SushiLayout::BeachShore + FVector(0.f, 400.f, -SushiLayout::BeachShore.Z);
		Where = LOCTEXT("LandedSea", "손님이 바다에 빠졌는디 갈매기가 해변으로 옮겨 놨슈. 구출하러 가유!");
	}
	SetActorLocation(Landing);
	SetActorRotation(FRotator(0.f, GetActorRotation().Yaw, 90.f)); // 대자로 뻗음
	CustomerState = ESushiCustomerState::Crashed;
	if (ASushiWildGameState* State = GetWorld()->GetGameState<ASushiWildGameState>())
	{
		State->PushToast(Where, FLinearColor(1.f, 0.85f, 0.4f), 4.f);
	}
}

bool ASushiCustomer::CanInteract(const ASushiChefCharacter* Chef) const
{
	return CustomerState == ESushiCustomerState::Crashed;
}

FText ASushiCustomer::GetPrompt(const ASushiChefCharacter* Chef) const
{
	return LOCTEXT("Rescue", "[E] 추락 손님 구출하기");
}

void ASushiCustomer::OnInteractPressed(ASushiChefCharacter* Chef)
{
	if (CustomerState != ESushiCustomerState::Crashed)
	{
		return;
	}
	if (ASushiWildGameState* State = GetWorld()->GetGameState<ASushiWildGameState>())
	{
		const bool bThrill = Kind == ESushiCustomerKind::Thrill;
		State->AddCoins(bThrill ? 80 : 30);
		State->ApplyReview(bThrill ? 5 : 4);
		State->PushToast(bThrill
			? LOCTEXT("RescuedThrill", "날다람G: 최고의 비행이었슈! 착지가 좀 아팠지만 또 올게유! (팁 +80)")
			: LOCTEXT("Rescued", "손님: 그 맛을… 잊을 수가 없어유. 단골 할게유! (사과 보너스 +30)"), FLinearColor(0.6f, 1.f, 0.6f), 5.f);
	}
	Destroy();
}

#undef LOCTEXT_NAMESPACE
