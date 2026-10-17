#include "CornBait.h"

#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "SushiShapes.h"

ACornBait::ACornBait()
{
	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	Collision->InitSphereRadius(12.f);
	Collision->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	Collision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	Collision->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	RootComponent = Collision;

	Movement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Movement"));
	Movement->UpdatedComponent = Collision;
	Movement->bAutoActivate = false;
	Movement->bShouldBounce = true;
	Movement->Bounciness = 0.25f;
	Movement->Friction = 0.6f;
	Movement->ProjectileGravityScale = 1.f;

	InitialLifeSpan = 25.f;
}

void ACornBait::BeginPlay()
{
	Super::BeginPlay();
	// 노란 옥수수 알갱이 + 초록 껍질
	SushiShapes::Add(this, Collision, ESushiShape::Cylinder, FVector::ZeroVector, FVector(0.14f, 0.14f, 0.3f), FLinearColor(1.f, 0.85f, 0.1f));
	SushiShapes::Add(this, Collision, ESushiShape::Cone, FVector(0.f, 0.f, -12.f), FVector(0.18f, 0.18f, 0.15f), FLinearColor(0.3f, 0.7f, 0.2f));
}

void ACornBait::Launch(const FVector& Velocity)
{
	Movement->Velocity = Velocity;
	Movement->Activate(true);
}

void ACornBait::Nibble(float DeltaSeconds)
{
	Remaining -= DeltaSeconds;
	if (Remaining <= 0.f)
	{
		Destroy();
	}
}

bool ACornBait::HasLanded() const
{
	return !Movement->IsActive() || Movement->Velocity.SizeSquared() < 400.f;
}
