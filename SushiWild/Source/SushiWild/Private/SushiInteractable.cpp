#include "SushiInteractable.h"

#include "Components/SceneComponent.h"

ASushiInteractable::ASushiInteractable()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}
