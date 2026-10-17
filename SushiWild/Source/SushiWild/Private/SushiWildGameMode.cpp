#include "SushiWildGameMode.h"

#include "Engine/World.h"
#include "GameFramework/PlayerStart.h"
#include "SushiChefCharacter.h"
#include "SushiHUD.h"
#include "SushiWild.h"
#include "SushiWildGameState.h"
#include "SushiWorldBuilder.h"

ASushiWildGameMode::ASushiWildGameMode()
{
	DefaultPawnClass = ASushiChefCharacter::StaticClass();
	HUDClass = ASushiHUD::StaticClass();
	GameStateClass = ASushiWildGameState::StaticClass();
}

void ASushiWildGameMode::StartPlay()
{
	// AGameMode 는 경기 시작(Super::StartPlay) 때 플레이어를 스폰하므로, 그 전에 섬부터 세운다.
	UWorld* World = GetWorld();
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	if (ASushiWorldBuilder* Builder = World->SpawnActor<ASushiWorldBuilder>(ASushiWorldBuilder::StaticClass(), FTransform::Identity, Params))
	{
		const FTransform Start = Builder->Build(GetGameState<ASushiWildGameState>());
		SpawnPoint = World->SpawnActor<APlayerStart>(APlayerStart::StaticClass(), Start, Params);
		UE_LOG(LogSushi, Log, TEXT("Sushi Wild island built. Player start at %s"), *Start.GetLocation().ToString());
	}

	Super::StartPlay();
}

AActor* ASushiWildGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	return SpawnPoint ? SpawnPoint.Get() : Super::ChoosePlayerStart_Implementation(Player);
}
