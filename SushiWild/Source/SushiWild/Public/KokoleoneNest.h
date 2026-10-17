#pragma once

#include "CoreMinimal.h"
#include "SushiInteractable.h"
#include "KokoleoneNest.generated.h"

class ASushiChefCharacter;
class ASushiWildGameState;
class UStaticMeshComponent;
class USushiVisionComponent;

// 미션 "꼬꼬레오네 패밀리의 알" (기획서 4-2).
// 마피아 보스 암탉 돈 꼬꼬레오네가 둥지 위에 앉아 좌우로 시선을 훑고, 20% 확률로 뒤돌아보는 척을 한다.
// 대부 엉덩이 밑의 알을 E 꾹 눌러 빼내면 등급(일반~전설)이 굴려진다.
UCLASS()
class SUSHIWILD_API AKokoleoneNest : public ASushiInteractable
{
	GENERATED_BODY()

public:
	AKokoleoneNest();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	virtual bool CanInteract(const ASushiChefCharacter* Chef) const override;
	virtual FText GetPrompt(const ASushiChefCharacter* Chef) const override;
	virtual void OnInteractPressed(ASushiChefCharacter* Chef) override;
	virtual void OnInteractReleased(ASushiChefCharacter* Chef) override;
	virtual void DrawMinigameHUD(ASushiHUD* HUD, UCanvas* Canvas) const override;

	UPROPERTY(EditAnywhere, Category = "Sushi|Kokoleone") float SweepAmplitudeDeg = 70.f;
	UPROPERTY(EditAnywhere, Category = "Sushi|Kokoleone") float SweepPeriodSeconds = 10.f;
	UPROPERTY(EditAnywhere, Category = "Sushi|Kokoleone") float FakeLookBackChance = 0.2f;
	UPROPERTY(EditAnywhere, Category = "Sushi|Kokoleone") float FakeLookBackSeconds = 1.5f;
	UPROPERTY(EditAnywhere, Category = "Sushi|Kokoleone") float PullSeconds = 2.5f;
	UPROPERTY(EditAnywhere, Category = "Sushi|Kokoleone") float AlarmSeconds = 30.f;
	UPROPERTY(EditAnywhere, Category = "Sushi|Kokoleone") float EggRespawnSeconds = 60.f;

	bool IsAlarmed() const { return AlarmTimeLeft > 0.f; }
	float GetSuspicion() const { return Suspicion; }
	float GetAlarmTimeLeft() const { return AlarmTimeLeft; }

	// 병아리 부하가 본 것을 보스에게 보고한다.
	void ReportSighting(float Amount);

protected:
	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> GazePivot;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USushiVisionComponent> Vision;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> EggMesh;

private:
	void BuildVisuals();
	void UpdateGaze(float DeltaSeconds, ASushiWildGameState* State, const ASushiChefCharacter* Chef);
	void UpdateSuspicion(float DeltaSeconds, ASushiWildGameState* State, ASushiChefCharacter* Chef);
	void CheckCornTaunt(ASushiWildGameState* State);
	void TriggerAlarm(ASushiWildGameState* State);
	void StopPulling(bool bFailed);
	void CompleteHeist();

	float Suspicion = 0.f;
	float AlarmTimeLeft = 0.f;
	float SweepTime = 0.f;
	int32 LastSweepCycle = 0;
	bool bFakeLookBack = false;
	float FakeLookBackLeft = 0.f;
	bool bSaidSuspicious = false;

	bool bHasEgg = true;
	float EggRespawnLeft = 0.f;
	bool bPulling = false;
	float PullProgress = 0.f;

	TWeakObjectPtr<ASushiChefCharacter> Puller;
	TWeakObjectPtr<AActor> LastTauntedCorn;
};
