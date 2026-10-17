#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "SushiHUD.generated.h"

class UFont;

// 에셋 없이 캔버스로 그리는 프로토타입 HUD.
// 한글이 네모(□)로 보이면 한글 폰트(예: Noto Sans KR)를 임포트해서 KoreanFontOverride 에 넣는다.
UCLASS()
class SUSHIWILD_API ASushiHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

	UPROPERTY(EditAnywhere, Category = "Sushi|HUD")
	TObjectPtr<UFont> KoreanFontOverride;

	void DrawLabel(const FText& Text, float X, float Y, const FLinearColor& Color, int32 Size = 18, bool bCenterX = false);
	void DrawBar(float X, float Y, float W, float H, float Fill01, const FLinearColor& Color, const FText& Label);
	void DrawPanel(float X, float Y, float W, float H, float Alpha = 0.55f);

private:
	void DrawStatus();
	void DrawChefBars();
	void DrawInventory();
	void DrawToasts();
	void DrawWorldLabels();
	void DrawCustomers();
	void DrawPromptAndMinigame();
	void DrawControls();

	// 화면 밖이면 false
	bool ProjectToScreen(const FVector& World, FVector2D& OutScreen) const;
};
