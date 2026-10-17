// 섬 지도 좌표 (기획서 2-1). 월드 빌더와 게임 로직이 같은 숫자를 쓰도록 한 곳에 모은다.
// 단위 cm. +X = 동쪽(해변), -X = 서쪽(농장), +Y = 북쪽(숲).
#pragma once

#include "CoreMinimal.h"

namespace SushiLayout
{
	constexpr float WorldHalfSize = 20000.f;      // 400m × 400m 회색 박스 섬

	inline const FVector Village(0.f, 0.f, 0.f);
	inline const FVector Counter(500.f, 0.f, 0.f);
	inline const FVector ShopFront(150.f, -250.f, 120.f); // 펠리컨 구급대가 '퉤' 뱉는 곳
	constexpr float ShopDeliveryRadius = 700.f;            // 알을 들고 여기 들어오면 배달 성공

	inline const FVector Farm(-8000.f, 0.f, 0.f);
	inline const FVector Beach(9000.f, 0.f, 0.f);
	inline const FVector Forest(0.f, 9000.f, 0.f);

	constexpr float ShoreX = 10500.f;             // 이 선 너머는 바다
	constexpr float PierHalfWidth = 180.f;
	constexpr float PierEndX = 12800.f;
	constexpr float PierDeckTop = 40.f;
	inline const FVector FishingSpot(12550.f, 0.f, PierDeckTop);
	inline const FVector BeachShore(9800.f, 0.f, 120.f);  // 바다에 빠지면 갈매기가 여기로 건져 줌
}
