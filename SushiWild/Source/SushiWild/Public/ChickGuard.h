#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ChickGuard.generated.h"

class ACornBait;
class AKokoleoneNest;
class ASushiChefCharacter;
class USphereComponent;
class USushiVisionComponent;

// 순찰 도는 부하 병아리 '삐약 형제'. 본 것은 대부에게 보고하고,
// 경보가 울리면 요리사를 쫓아가 쫀다. 옥수수를 던지면 우르르 몰려간다 (기획서 4-2).
UCLASS()
class SUSHIWILD_API AChickGuard : public AActor
{
	GENERATED_BODY()

public:
	AChickGuard();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	void SetupPatrol(AKokoleoneNest* InBoss, const FVector& InCenter, float InRadius, float InAngularSpeed, float InStartAngle);

	UPROPERTY(EditAnywhere, Category = "Sushi|Chick") float PatrolSpeed = 380.f;
	UPROPERTY(EditAnywhere, Category = "Sushi|Chick") float ChaseSpeed = 650.f;
	UPROPERTY(EditAnywhere, Category = "Sushi|Chick") float PeckDamage = 8.f;
	UPROPERTY(EditAnywhere, Category = "Sushi|Chick") float GiveUpDistance = 3500.f;

protected:
	UPROPERTY(VisibleAnywhere) TObjectPtr<USphereComponent> Body;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USushiVisionComponent> Vision;

private:
	void BuildVisuals();
	ACornBait* FindCorn(float MaxDistance) const;
	// 목표 쪽으로 걸어간다. 도착하면 true.
	bool MoveToward(const FVector& Target, float Speed, float DeltaSeconds, float AcceptRadius);

	TWeakObjectPtr<AKokoleoneNest> Boss;
	FVector Center = FVector::ZeroVector;
	float Radius = 800.f;
	float AngularSpeed = 0.3f;
	float Angle = 0.f;
	float PeckCooldown = 0.f;
};
