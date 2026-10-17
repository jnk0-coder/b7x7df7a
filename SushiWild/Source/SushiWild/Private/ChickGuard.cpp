#include "ChickGuard.h"

#include "Components/SphereComponent.h"
#include "CornBait.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "KokoleoneNest.h"
#include "SushiChefCharacter.h"
#include "SushiShapes.h"
#include "SushiVisionComponent.h"
#include "SushiWildGameState.h"

namespace
{
	constexpr float ChickZ = 36.f; // 몸 반지름 30 + 바닥 패치 두께 여유
}

AChickGuard::AChickGuard()
{
	PrimaryActorTick.bCanEverTick = true;

	Body = CreateDefaultSubobject<USphereComponent>(TEXT("Body"));
	Body->InitSphereRadius(30.f);
	Body->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	Body->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	Body->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
	RootComponent = Body;

	Vision = CreateDefaultSubobject<USushiVisionComponent>(TEXT("Vision"));
	Vision->SetupAttachment(Body);
	Vision->SetRelativeLocation(FVector(30.f, 0.f, 30.f));
	Vision->Range = 900.f;
	Vision->HalfAngleDeg = 30.f;
	Vision->HearingRange = 600.f;
	Vision->ConeSegments = 10;
}

void AChickGuard::BeginPlay()
{
	Super::BeginPlay();
	BuildVisuals();
}

void AChickGuard::BuildVisuals()
{
	const FLinearColor Yellow(1.f, 0.85f, 0.15f);
	const FLinearColor Orange(1.f, 0.55f, 0.1f);
	SushiShapes::Add(this, Body, ESushiShape::Sphere, FVector::ZeroVector, FVector(0.6f, 0.55f, 0.55f), Yellow);
	SushiShapes::Add(this, Body, ESushiShape::Sphere, FVector(18.f, 0.f, 32.f), FVector(0.4f), Yellow);
	SushiShapes::Add(this, Body, ESushiShape::Cone, FVector(40.f, 0.f, 29.f), FVector(0.12f), Orange, false, FRotator(-90.f, 0.f, 0.f));
	// 마피아 졸개 선글라스
	SushiShapes::Add(this, Body, ESushiShape::Cube, FVector(34.f, 0.f, 38.f), FVector(0.05f, 0.32f, 0.07f), FLinearColor(0.02f, 0.02f, 0.02f));
	SushiShapes::Add(this, Body, ESushiShape::Cylinder, FVector(0.f, 10.f, -28.f), FVector(0.05f, 0.05f, 0.18f), Orange);
	SushiShapes::Add(this, Body, ESushiShape::Cylinder, FVector(0.f, -10.f, -28.f), FVector(0.05f, 0.05f, 0.18f), Orange);
}

void AChickGuard::SetupPatrol(AKokoleoneNest* InBoss, const FVector& InCenter, float InRadius, float InAngularSpeed, float InStartAngle)
{
	Boss = InBoss;
	Center = InCenter;
	Radius = InRadius;
	AngularSpeed = InAngularSpeed;
	Angle = InStartAngle;
}

void AChickGuard::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	PeckCooldown = FMath::Max(0.f, PeckCooldown - DeltaSeconds);

	ASushiWildGameState* State = GetWorld()->GetGameState<ASushiWildGameState>();
	ASushiChefCharacter* Chef = Cast<ASushiChefCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
	AKokoleoneNest* Nest = Boss.Get();
	const float RangeMult = State ? State->GetVisionRangeMultiplier() : 1.f;
	const float NoiseMult = State ? State->GetNoiseMultiplier() : 1.f;
	Vision->SetRangeMultiplier(RangeMult);

	// 1) 옥수수가 보이면 경보 중이어도 우르르
	if (ACornBait* Corn = FindCorn(1200.f))
	{
		if (MoveToward(Corn->GetActorLocation(), ChaseSpeed * 0.8f, DeltaSeconds, 50.f))
		{
			Corn->Nibble(DeltaSeconds);
		}
		Vision->SetAlertLevel(0.f);
		return;
	}

	// 2) 경보 중이면 요리사를 쫓아가 쫀다
	const bool bChase = Nest && Nest->IsAlarmed() && Chef && !Chef->IsKnockedOut()
		&& FVector::Dist2D(Chef->GetActorLocation(), Center) < GiveUpDistance;
	if (bChase)
	{
		Vision->SetAlertLevel(1.f);
		if (MoveToward(Chef->GetActorLocation(), ChaseSpeed, DeltaSeconds, 110.f) && PeckCooldown <= 0.f)
		{
			Chef->ApplyPeck(PeckDamage, this);
			PeckCooldown = 0.8f;
		}
		return;
	}

	// 3) 평소엔 둥지 둘레를 순찰
	Angle += AngularSpeed * DeltaSeconds;
	const FVector OnCircle = Center + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.f) * Radius;
	MoveToward(OnCircle, PatrolSpeed, DeltaSeconds, 5.f);

	// 본 것·들은 것은 대부에게 보고
	if (Nest && Chef)
	{
		const float Sight = Vision->ComputeSight(Chef, RangeMult);
		const float Hearing = Vision->ComputeHearing(Chef, NoiseMult);
		if (Sight > 0.f || Hearing > 0.f)
		{
			Nest->ReportSighting((Sight * 0.9f + Hearing * 0.5f) * DeltaSeconds);
		}
		Vision->SetAlertLevel(Nest->GetSuspicion() * 0.9f);
	}
}

ACornBait* AChickGuard::FindCorn(float MaxDistance) const
{
	ACornBait* Best = nullptr;
	float BestDistSq = FMath::Square(MaxDistance);
	for (TActorIterator<ACornBait> It(GetWorld()); It; ++It)
	{
		if (!It->HasLanded())
		{
			continue;
		}
		const float DistSq = FVector::DistSquared2D(It->GetActorLocation(), GetActorLocation());
		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			Best = *It;
		}
	}
	return Best;
}

bool AChickGuard::MoveToward(const FVector& Target, float Speed, float DeltaSeconds, float AcceptRadius)
{
	const FVector Here = GetActorLocation();
	FVector ToTarget = Target - Here;
	ToTarget.Z = 0.f;
	const float Dist = ToTarget.Size();
	if (Dist <= AcceptRadius)
	{
		if (Dist > KINDA_SMALL_NUMBER)
		{
			SetActorRotation(ToTarget.Rotation());
		}
		return true;
	}

	const FVector Dir = ToTarget / Dist;
	FVector Delta = Dir * FMath::Min(Speed * DeltaSeconds, Dist - AcceptRadius);
	Delta.Z = ChickZ - Here.Z;

	FHitResult Hit;
	SetActorLocation(Here + Delta, true, &Hit);
	if (Hit.bBlockingHit)
	{
		// 건초더미에 막히면 옆으로 미끄러져 돌아간다.
		const FVector Slide = FVector::VectorPlaneProject(Delta * (1.f - Hit.Time), Hit.Normal);
		SetActorLocation(GetActorLocation() + Slide, true);
	}
	SetActorRotation(Dir.Rotation());
	return false;
}
