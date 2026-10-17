#include "SushiVisionComponent.h"

#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "ProceduralMeshComponent.h"
#include "SushiChefCharacter.h"
#include "SushiShapes.h"

namespace
{
	const FLinearColor CalmColor(1.f, 0.9f, 0.35f);
	const FLinearColor SuspiciousColor(1.f, 0.55f, 0.1f);
	const FLinearColor AlarmColor(1.f, 0.12f, 0.1f);
}

USushiVisionComponent::USushiVisionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void USushiVisionComponent::BeginPlay()
{
	Super::BeginPlay();

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	Fan = NewObject<UProceduralMeshComponent>(Owner);
	Fan->SetupAttachment(this);
	Fan->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Fan->SetCastShadow(false);
	Fan->RegisterComponent();
	Owner->AddInstanceComponent(Fan);

	// 땅바닥(z=0) 바로 위에 눕힌다.
	Fan->SetRelativeLocation(FVector(0.f, 0.f, -GetComponentLocation().Z + 4.f));

	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	Vertices.Add(FVector::ZeroVector);
	const float Half = FMath::DegreesToRadians(HalfAngleDeg);
	for (int32 i = 0; i <= ConeSegments; ++i)
	{
		const float A = -Half + 2.f * Half * static_cast<float>(i) / static_cast<float>(ConeSegments);
		Vertices.Add(FVector(FMath::Cos(A), FMath::Sin(A), 0.f) * Range);
	}
	for (int32 i = 0; i < ConeSegments; ++i)
	{
		// 앞뒷면을 모두 넣어서 어느 쪽에서 봐도 보이게 한다.
		Triangles.Append({ 0, i + 1, i + 2 });
		Triangles.Append({ 0, i + 2, i + 1 });
	}
	Fan->CreateMeshSection_LinearColor(0, Vertices, Triangles, TArray<FVector>(), TArray<FVector2D>(),
		TArray<FLinearColor>(), TArray<FProcMeshTangent>(), false);

	FanMaterial = SushiShapes::MakeColorMaterial(this, CalmColor);
	Fan->SetMaterial(0, FanMaterial);
}

float USushiVisionComponent::ComputeSight(const ASushiChefCharacter* Chef, float RangeMultiplier) const
{
	if (!Chef || Chef->IsKnockedOut())
	{
		return 0.f;
	}

	const FVector Eye = GetComponentLocation();
	const FVector Target = Chef->GetActorLocation();
	const FVector To = Target - Eye;
	const float Dist = To.Size2D();
	const float EffectiveRange = Range * RangeMultiplier;
	if (Dist > EffectiveRange)
	{
		return 0.f;
	}

	const FVector Forward = GetForwardVector().GetSafeNormal2D();
	const FVector Dir = FVector(To.X, To.Y, 0.f).GetSafeNormal();
	if (FVector::DotProduct(Forward, Dir) < FMath::Cos(FMath::DegreesToRadians(HalfAngleDeg)))
	{
		return 0.f;
	}

	// 건초더미·상자 뒤에 숨으면 시선이 막힌다.
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(SushiSight), false, GetOwner());
	if (GetWorld()->LineTraceSingleByChannel(Hit, Eye, Target, ECC_Visibility, Params) && Hit.GetActor() != Chef)
	{
		return 0.f;
	}

	const float Closeness = 1.f - Dist / EffectiveRange;
	return (0.4f + 0.6f * Closeness) * Chef->GetVisibilityFactor();
}

float USushiVisionComponent::ComputeHearing(const ASushiChefCharacter* Chef, float NoiseMultiplier) const
{
	if (!Chef || Chef->IsKnockedOut())
	{
		return 0.f;
	}
	const float Dist = FVector::Dist2D(GetComponentLocation(), Chef->GetActorLocation());
	if (Dist > HearingRange)
	{
		return 0.f;
	}
	return Chef->GetNoise() * NoiseMultiplier * (1.f - Dist / HearingRange);
}

void USushiVisionComponent::SetAlertLevel(float Alert01)
{
	const float Alert = FMath::Clamp(Alert01, 0.f, 1.f);
	if (!FanMaterial || FMath::IsNearlyEqual(Alert, LastAlert, 0.01f))
	{
		return;
	}
	LastAlert = Alert;
	const FLinearColor Color = Alert < 0.5f
		? FMath::Lerp(CalmColor, SuspiciousColor, Alert * 2.f)
		: FMath::Lerp(SuspiciousColor, AlarmColor, (Alert - 0.5f) * 2.f);
	FanMaterial->SetVectorParameterValue(TEXT("Color"), Color);
}

void USushiVisionComponent::SetRangeMultiplier(float Multiplier)
{
	if (Fan)
	{
		Fan->SetRelativeScale3D(FVector(Multiplier, Multiplier, 1.f));
	}
}
