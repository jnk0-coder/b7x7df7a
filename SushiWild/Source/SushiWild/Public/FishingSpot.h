#pragma once

#include "CoreMinimal.h"
#include "SushiInteractable.h"
#include "FishingSpot.generated.h"

// 미션 "낚싯줄 위의 콘서트" (기획서 4-2 리듬 낚시).
// 생선마다 장르와 BPM 이 다르다. 노트를 놓치면 줄 텐션이 올라가고, 100이 되면 줄이 끊긴다.
UCLASS()
class SUSHIWILD_API AFishingSpot : public ASushiInteractable
{
	GENERATED_BODY()

public:
	AFishingSpot();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	virtual FText GetPrompt(const ASushiChefCharacter* Chef) const override;
	virtual void OnInteractPressed(ASushiChefCharacter* Chef) override;
	virtual void DrawMinigameHUD(ASushiHUD* HUD, UCanvas* Canvas) const override;

	UPROPERTY(EditAnywhere, Category = "Sushi|Fishing") int32 NoteCount = 8;
	UPROPERTY(EditAnywhere, Category = "Sushi|Fishing") int32 LeadInBeats = 3;

private:
	void StartFishing(ASushiChefCharacter* Chef);
	void Judge(double Now);
	void Finish(bool bCaught);
	double NoteTime(int32 Index) const { return StartTime + Index * BeatSeconds; }

	TWeakObjectPtr<ASushiChefCharacter> Angler;
	bool bActive = false;
	int32 FishIndex = 0;
	double StartTime = 0.0;
	double BeatSeconds = 0.5;
	int32 NextNote = 0;
	float Tension = 30.f;
	float WindowScale = 1.f;
	int32 Perfects = 0;
	int32 Goods = 0;
	int32 Misses = 0;

	FText LastJudgeText;
	FLinearColor LastJudgeColor = FLinearColor::White;
	double LastJudgeTime = -10.0;
};
