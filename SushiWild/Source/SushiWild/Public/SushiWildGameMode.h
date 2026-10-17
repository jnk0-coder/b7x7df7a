#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "SushiWildGameMode.generated.h"

class APlayerStart;

// 경기 시작 전에 섬을 세우고(ASushiWorldBuilder), 요리사를 포장마차 앞에 내려놓는다.
UCLASS()
class SUSHIWILD_API ASushiWildGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	ASushiWildGameMode();

	virtual void StartPlay() override;
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;

private:
	UPROPERTY() TObjectPtr<APlayerStart> SpawnPoint;
};
