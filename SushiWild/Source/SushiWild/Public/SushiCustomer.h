#pragma once

#include "CoreMinimal.h"
#include "SushiInteractable.h"
#include "SushiTypes.h"
#include "SushiCustomer.generated.h"

UENUM()
enum class ESushiCustomerKind : uint8
{
	Regular,   // 동네 손님
	Thrill,    // 익스트림 다람쥐 '날다람G' — 로켓 발사를 원한다 (기획서 5-4)
	Picky      // 비주얼 중시 예술가 손님
};

UENUM()
enum class ESushiCustomerState : uint8
{
	Waiting,
	Leaving,
	Flying,    // 와사비 로켓 발사 중
	Crashed    // 어딘가에 떨어져 구출을 기다리는 중
};

// 카운터 앞 손님. 기다리다 지치면 의자를 뜯어 먹고 가고(기획서 7-1 #8),
// 와사비를 85% 넘게 넣으면 하늘로 날아가 맵 어딘가에 떨어진다.
UCLASS()
class SUSHIWILD_API ASushiCustomer : public ASushiInteractable
{
	GENERATED_BODY()

public:
	ASushiCustomer();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	virtual bool CanInteract(const ASushiChefCharacter* Chef) const override;
	virtual FText GetPrompt(const ASushiChefCharacter* Chef) const override;
	virtual void OnInteractPressed(ASushiChefCharacter* Chef) override;

	void InitCustomer(ESushiCustomerKind InKind, FRandomStream& Rng);

	ESushiCustomerKind GetKind() const { return Kind; }
	ESushiCustomerState GetState() const { return CustomerState; }
	bool IsWaiting() const { return CustomerState == ESushiCustomerState::Waiting; }
	bool IsCrashed() const { return CustomerState == ESushiCustomerState::Crashed; }
	const FSushiFlavor& GetTaste() const { return Taste; }
	FText GetDisplayName() const;
	float GetPatience01() const { return MaxPatience > 0.f ? Patience / MaxPatience : 0.f; }

	void LeaveHappy();
	void LaunchRocket(FRandomStream& Rng);

	UPROPERTY(EditAnywhere, Category = "Sushi|Customer") float MaxPatience = 150.f;

private:
	void BuildVisuals();
	void TickFlight(float DeltaSeconds);

	ESushiCustomerKind Kind = ESushiCustomerKind::Regular;
	ESushiCustomerState CustomerState = ESushiCustomerState::Waiting;
	FSushiFlavor Taste;
	FLinearColor ShirtColor = FLinearColor(0.3f, 0.6f, 0.9f);
	float Patience = 150.f;
	float LeaveTimeLeft = 0.f;
	FVector Velocity = FVector::ZeroVector;
};
