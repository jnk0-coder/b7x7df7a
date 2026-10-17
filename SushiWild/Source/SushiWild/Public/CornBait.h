#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CornBait.generated.h"

class UProjectileMovementComponent;
class USphereComponent;

// 던지는 옥수수 미끼. 병아리는 우르르 몰려가지만 대부는 안 넘어간다 (기획서 4-2).
UCLASS()
class SUSHIWILD_API ACornBait : public AActor
{
	GENERATED_BODY()

public:
	ACornBait();

	virtual void BeginPlay() override;

	void Launch(const FVector& Velocity);

	// 병아리가 쪼아 먹는 중. 다 먹으면 사라진다.
	void Nibble(float DeltaSeconds);

	bool HasLanded() const;

protected:
	UPROPERTY(VisibleAnywhere) TObjectPtr<USphereComponent> Collision;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UProjectileMovementComponent> Movement;

private:
	float Remaining = 3.f; // 병아리 한 마리 기준 3초면 다 먹는다
};
