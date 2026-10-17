#pragma once

#include "CoreMinimal.h"
#include "SushiBalance.h"
#include "SushiInteractable.h"
#include "SushiTypes.h"
#include "SushiCounter.generated.h"

class ASushiCustomer;

UENUM()
enum class ESushiCookPhase : uint8
{
	Idle,
	Shari,   // 밥알 리듬 액션 (기획서 5-3)
	Wasabi,  // 와사비 룰렛 (기획서 5-4)
	Result
};

// 골판지 포장마차의 주방 카운터. 손님을 받고, 조리 미니게임을 돌리고, 채점한다 (기획서 5장).
UCLASS()
class SUSHIWILD_API ASushiCounter : public ASushiInteractable
{
	GENERATED_BODY()

public:
	ASushiCounter();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	virtual FText GetPrompt(const ASushiChefCharacter* Chef) const override;
	virtual void OnInteractPressed(ASushiChefCharacter* Chef) override;
	virtual void DrawMinigameHUD(ASushiHUD* HUD, UCanvas* Canvas) const override;

	UPROPERTY(EditAnywhere, Category = "Sushi|Kitchen") float ShariBpm = 100.f;
	UPROPERTY(EditAnywhere, Category = "Sushi|Kitchen") float WasabiPeriodSeconds = 1.6f;
	UPROPERTY(EditAnywhere, Category = "Sushi|Kitchen") float RussianWasabiChance = 0.1f;
	// 물리 플레이팅은 v0.2 에서. 지금은 고정 점수.
	UPROPERTY(EditAnywhere, Category = "Sushi|Kitchen") float PlatingScore = 70.f;
	UPROPERTY(EditAnywhere, Category = "Sushi|Kitchen") float CustomerIntervalMin = 15.f;
	UPROPERTY(EditAnywhere, Category = "Sushi|Kitchen") float CustomerIntervalMax = 25.f;

private:
	static constexpr int32 NumSeats = 2;
	static constexpr int32 ShariBeats = 4;

	void BuildVisuals();
	void TrySpawnCustomer(float DeltaSeconds);
	ASushiCustomer* FindWaitingCustomer() const;

	void StartCooking(ASushiChefCharacter* Chef);
	void PressShari(double Now);
	void FinishShari();
	void PressWasabi(double Now);
	void Serve();
	void EndCooking();

	double ShariBeatTime(int32 Index) const { return ShariStart + Index * (60.0 / ShariBpm); }
	float CurrentWasabiNeedle(double Now) const;

	UPROPERTY() TArray<TObjectPtr<ASushiCustomer>> Seats;
	float SpawnTimer = 5.f;

	TWeakObjectPtr<ASushiChefCharacter> Cook;
	TWeakObjectPtr<ASushiCustomer> Target;
	FSushiIngredient Ingredient;
	ESushiCookPhase Phase = ESushiCookPhase::Idle;

	double ShariStart = 0.0;
	int32 ShariNext = 0;
	float ShariErrors[ShariBeats] = { 0.f, 0.f, 0.f, 0.f };
	SushiBalance::EShariGrade ShariGrade = SushiBalance::ShariPerfect;
	float ShariScoreValue = 0.f;

	double WasabiStart = 0.0;
	float WasabiPeriod = 1.6f;
	bool bRussianWasabi = false;
	float WasabiAmount = 0.f;

	float ResultScore = 0.f;
	int32 ResultStars = 0;
	int32 ResultCoins = 0;
	float ResultTimeLeft = 0.f;
};
