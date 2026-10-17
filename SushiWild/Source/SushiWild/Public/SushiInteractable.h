#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SushiInteractable.generated.h"

class ASushiChefCharacter;
class ASushiHUD;
class UCanvas;

// [E] 로 상호작용하는 모든 것(둥지, 낚시터, 주방 카운터, 추락 손님)의 부모.
// 미니게임 중에는 요리사가 이 액터에 '잠기고', E 입력이 전부 여기로 온다.
UCLASS(Abstract)
class SUSHIWILD_API ASushiInteractable : public AActor
{
	GENERATED_BODY()

public:
	ASushiInteractable();

	// 이 거리(cm) 안에 들어오면 프롬프트가 뜬다.
	UPROPERTY(EditAnywhere, Category = "Sushi|Interact")
	float InteractRadius = 250.f;

	virtual bool CanInteract(const ASushiChefCharacter* Chef) const { return true; }
	virtual FText GetPrompt(const ASushiChefCharacter* Chef) const { return FText::GetEmpty(); }

	virtual void OnInteractPressed(ASushiChefCharacter* Chef) {}
	virtual void OnInteractReleased(ASushiChefCharacter* Chef) {}

	// 미니게임이 돌아가는 동안 HUD 가 매 프레임 불러 준다.
	virtual void DrawMinigameHUD(ASushiHUD* HUD, UCanvas* Canvas) const {}
};
