#include "SushiChefCharacter.h"

#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "CornBait.h"
#include "Engine/LocalPlayer.h"
#include "EngineUtils.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "SushiBalance.h"
#include "SushiCatalog.h"
#include "SushiInteractable.h"
#include "SushiShapes.h"
#include "SushiWildGameState.h"
#include "SushiWorldLayout.h"
#include "TimerManager.h"

#define LOCTEXT_NAMESPACE "SushiChef"

namespace
{
	constexpr float StandingHalfHeight = 96.f;
}

ASushiChefCharacter::ASushiChefCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->InitCapsuleSize(42.f, StandingHalfHeight);

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	UCharacterMovementComponent* CharMove = GetCharacterMovement();
	CharMove->bOrientRotationToMovement = true;
	CharMove->RotationRate = FRotator(0.f, 540.f, 0.f);
	CharMove->MaxWalkSpeed = WalkSpeed;
	CharMove->MaxWalkSpeedCrouched = 220.f;
	CharMove->JumpZVelocity = 600.f;
	CharMove->AirControl = 0.35f;
	CharMove->GetNavAgentPropertiesRef().bCanCrouch = true;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 480.f;
	CameraBoom->SocketOffset = FVector(0.f, 0.f, 90.f);
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	VisualRoot = CreateDefaultSubobject<USceneComponent>(TEXT("VisualRoot"));
	VisualRoot->SetupAttachment(RootComponent);
}

void ASushiChefCharacter::BeginPlay()
{
	Super::BeginPlay();
	Health = MaxHealth;
	BuildVisuals();
}

void ASushiChefCharacter::BuildVisuals()
{
	const FLinearColor White(0.95f, 0.95f, 0.92f);
	const FLinearColor Skin(1.f, 0.8f, 0.65f);
	const FLinearColor Steel(0.7f, 0.72f, 0.75f);

	// 몸통 + 앞치마
	SushiShapes::Add(this, VisualRoot, ESushiShape::Cylinder, FVector(0.f, 0.f, -36.f), FVector(0.75f, 0.75f, 1.2f), White);
	SushiShapes::Add(this, VisualRoot, ESushiShape::Cube, FVector(34.f, 0.f, -50.f), FVector(0.1f, 0.6f, 0.8f), FLinearColor(0.15f, 0.3f, 0.7f));
	// 머리 + 눈 + 요리사 모자
	SushiShapes::Add(this, VisualRoot, ESushiShape::Sphere, FVector(0.f, 0.f, 55.f), FVector(0.6f), Skin);
	SushiShapes::Add(this, VisualRoot, ESushiShape::Sphere, FVector(26.f, -11.f, 62.f), FVector(0.08f), FLinearColor::Black);
	SushiShapes::Add(this, VisualRoot, ESushiShape::Sphere, FVector(26.f, 11.f, 62.f), FVector(0.08f), FLinearColor::Black);
	SushiShapes::Add(this, VisualRoot, ESushiShape::Cylinder, FVector(0.f, 0.f, 105.f), FVector(0.5f, 0.5f, 0.55f), White);
	SushiShapes::Add(this, VisualRoot, ESushiShape::Sphere, FVector(0.f, 0.f, 135.f), FVector(0.65f, 0.65f, 0.35f), White);
	// 등에 멘, 몸보다 큰 식칼 (넘어지면 칼이 먼저 착지한다)
	SushiShapes::Add(this, VisualRoot, ESushiShape::Cube, FVector(-42.f, 0.f, 30.f), FVector(0.04f, 0.45f, 2.1f), Steel, false, FRotator(0.f, 0.f, 18.f));
	SushiShapes::Add(this, VisualRoot, ESushiShape::Cylinder, FVector(-42.f, -22.f, -85.f), FVector(0.12f, 0.12f, 0.45f), FLinearColor(0.35f, 0.2f, 0.1f), false, FRotator(0.f, 0.f, 18.f));

	// 품에 안은 알 (평소엔 숨김)
	CarriedEggMesh = SushiShapes::Add(this, VisualRoot, ESushiShape::Sphere, FVector(48.f, 0.f, -5.f), FVector(0.28f, 0.28f, 0.36f), White);
	CarriedEggMesh->SetVisibility(false);
}

void ASushiChefCharacter::BuildInput()
{
	if (MappingContext)
	{
		return;
	}

	MappingContext = NewObject<UInputMappingContext>(this, TEXT("IMC_Chef"));

	auto MakeAction = [this](const TCHAR* Name, EInputActionValueType Type)
	{
		UInputAction* Action = NewObject<UInputAction>(this, Name);
		Action->ValueType = Type;
		return Action;
	};
	MoveAction = MakeAction(TEXT("IA_Move"), EInputActionValueType::Axis2D);
	LookAction = MakeAction(TEXT("IA_Look"), EInputActionValueType::Axis2D);
	JumpAction = MakeAction(TEXT("IA_Jump"), EInputActionValueType::Boolean);
	SprintAction = MakeAction(TEXT("IA_Sprint"), EInputActionValueType::Boolean);
	CrouchAction = MakeAction(TEXT("IA_Crouch"), EInputActionValueType::Boolean);
	InteractAction = MakeAction(TEXT("IA_Interact"), EInputActionValueType::Boolean);
	ThrowAction = MakeAction(TEXT("IA_Throw"), EInputActionValueType::Boolean);

	auto Map = [this](UInputAction* Action, const FKey& Key, bool bSwizzleYX = false, bool bNegate = false)
	{
		FEnhancedActionKeyMapping& Mapping = MappingContext->MapKey(Action, Key);
		if (bSwizzleYX)
		{
			Mapping.Modifiers.Add(NewObject<UInputModifierSwizzleAxis>(MappingContext));
		}
		if (bNegate)
		{
			Mapping.Modifiers.Add(NewObject<UInputModifierNegate>(MappingContext));
		}
	};

	// 이동: W/S 는 Y축, A/D 는 X축
	Map(MoveAction, EKeys::W, true);
	Map(MoveAction, EKeys::S, true, true);
	Map(MoveAction, EKeys::D);
	Map(MoveAction, EKeys::A, false, true);
	Map(MoveAction, EKeys::Gamepad_Left2D);

	Map(LookAction, EKeys::Mouse2D);
	Map(LookAction, EKeys::Gamepad_Right2D);

	Map(JumpAction, EKeys::SpaceBar);
	Map(JumpAction, EKeys::Gamepad_FaceButton_Bottom);

	Map(SprintAction, EKeys::LeftShift);
	Map(SprintAction, EKeys::Gamepad_LeftThumbstick);

	Map(CrouchAction, EKeys::LeftControl);
	Map(CrouchAction, EKeys::C);
	Map(CrouchAction, EKeys::Gamepad_FaceButton_Right);

	Map(InteractAction, EKeys::E);
	Map(InteractAction, EKeys::LeftMouseButton);
	Map(InteractAction, EKeys::Gamepad_FaceButton_Left);

	Map(ThrowAction, EKeys::Q);
	Map(ThrowAction, EKeys::Gamepad_RightShoulder);
}

void ASushiChefCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	BuildInput();

	UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!Input)
	{
		return;
	}
	Input->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ASushiChefCharacter::Move);
	Input->BindAction(LookAction, ETriggerEvent::Triggered, this, &ASushiChefCharacter::Look);
	Input->BindAction(JumpAction, ETriggerEvent::Started, this, &ASushiChefCharacter::StartJump);
	Input->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
	Input->BindAction(SprintAction, ETriggerEvent::Started, this, &ASushiChefCharacter::SprintStarted);
	Input->BindAction(SprintAction, ETriggerEvent::Completed, this, &ASushiChefCharacter::SprintStopped);
	Input->BindAction(CrouchAction, ETriggerEvent::Started, this, &ASushiChefCharacter::ToggleCrouch);
	Input->BindAction(InteractAction, ETriggerEvent::Started, this, &ASushiChefCharacter::InteractStarted);
	Input->BindAction(InteractAction, ETriggerEvent::Completed, this, &ASushiChefCharacter::InteractCompleted);
	Input->BindAction(ThrowAction, ETriggerEvent::Started, this, &ASushiChefCharacter::ThrowCorn);
}

void ASushiChefCharacter::PawnClientRestart()
{
	Super::PawnClientRestart();
	BuildInput();

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			Subsystem->ClearAllMappings();
			Subsystem->AddMappingContext(MappingContext, 0);
		}
	}
}

void ASushiChefCharacter::Move(const FInputActionValue& Value)
{
	if (MinigameOwner || bKnockedOut || !Controller)
	{
		return;
	}
	const FVector2D Axis = Value.Get<FVector2D>();
	const FRotator YawRotation(0.f, Controller->GetControlRotation().Yaw, 0.f);
	AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X), Axis.Y);
	AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y), Axis.X);
}

void ASushiChefCharacter::Look(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();
	AddControllerYawInput(Axis.X);
	AddControllerPitchInput(-Axis.Y);
}

void ASushiChefCharacter::StartJump()
{
	if (!MinigameOwner && !bKnockedOut)
	{
		Jump();
	}
}

void ASushiChefCharacter::OnJumped_Implementation()
{
	Super::OnJumped_Implementation();
	Noise = FMath::Max(Noise, 0.7f);
	if (bCarryingEgg)
	{
		EggWobble += 0.3f; // 알 들고 점프는 위험하다
	}
}

void ASushiChefCharacter::SprintStarted()
{
	bWantsSprint = true;
}

void ASushiChefCharacter::SprintStopped()
{
	bWantsSprint = false;
}

void ASushiChefCharacter::ToggleCrouch()
{
	if (MinigameOwner || bKnockedOut)
	{
		return;
	}
	if (bIsCrouched)
	{
		UnCrouch();
	}
	else
	{
		Crouch();
	}
}

void ASushiChefCharacter::OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	Super::OnStartCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);
	// 납작하게 찌그러져서 숨는다. 발은 바닥에 붙인 채로.
	const float Squash = 0.6f;
	const float NewHalfHeight = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	VisualRoot->SetRelativeScale3D(FVector(1.15f, 1.15f, Squash));
	VisualRoot->SetRelativeLocation(FVector(0.f, 0.f, StandingHalfHeight * Squash - NewHalfHeight));
}

void ASushiChefCharacter::OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	Super::OnEndCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);
	VisualRoot->SetRelativeScale3D(FVector::OneVector);
	VisualRoot->SetRelativeLocation(FVector::ZeroVector);
}

void ASushiChefCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	PeckToastCooldown = FMath::Max(0.f, PeckToastCooldown - DeltaSeconds);
	BagFullToastCooldown = FMath::Max(0.f, BagFullToastCooldown - DeltaSeconds);

	if (bKnockedOut)
	{
		return;
	}
	UpdateNoiseAndStamina(DeltaSeconds);
	UpdateEgg(DeltaSeconds);
	UpdateFocus();
	CheckSea();

	// 가게에 오면 옥수수 미끼를 다시 챙긴다.
	if (CornCount < 5 && FVector::Dist2D(GetActorLocation(), SushiLayout::Counter) < SushiLayout::ShopDeliveryRadius)
	{
		CornCount = 5;
	}
}

float ASushiChefCharacter::GetVisibilityFactor() const
{
	if (bIsCrouched)
	{
		return 0.5f;
	}
	return GetVelocity().Size2D() > WalkSpeed * 1.1f ? 1.2f : 1.f;
}

void ASushiChefCharacter::UpdateNoiseAndStamina(float DeltaSeconds)
{
	const float Speed = GetVelocity().Size2D();
	float TargetNoise = 0.f;
	if (Speed > 10.f)
	{
		TargetNoise = bIsCrouched ? 0.15f : FMath::Lerp(0.2f, 1.f, FMath::Clamp(Speed / SprintSpeed, 0.f, 1.f));
	}
	if (GetCharacterMovement()->IsFalling())
	{
		TargetNoise = FMath::Max(TargetNoise, 0.5f);
	}
	Noise = FMath::FInterpTo(Noise, TargetNoise, DeltaSeconds, 6.f);

	const bool bSprinting = bWantsSprint && !bIsCrouched && Speed > WalkSpeed * 0.9f;
	Stamina = FMath::Clamp(Stamina + (bSprinting ? -20.f : 12.f) * DeltaSeconds, 0.f, 100.f);

	float MaxSpeed = (bWantsSprint && Stamina > 1.f && !bIsCrouched) ? SprintSpeed : WalkSpeed;
	if (bCarryingEgg)
	{
		MaxSpeed *= 0.85f;
	}
	GetCharacterMovement()->MaxWalkSpeed = MaxSpeed;
}

void ASushiChefCharacter::StartCarryingEgg(const FSushiIngredient& Egg)
{
	CarriedEgg = Egg;
	bCarryingEgg = true;
	EggWobble = 0.f;
	CarriedEggMesh->SetVisibility(true);
	SushiShapes::SetColor(CarriedEggMesh, Egg.Rarity == ESushiRarity::Legend ? FLinearColor(1.f, 0.8f, 0.1f) : FLinearColor(0.95f, 0.93f, 0.85f));
}

void ASushiChefCharacter::UpdateEgg(float DeltaSeconds)
{
	if (!bCarryingEgg)
	{
		return;
	}

	// 걷기는 안전, 전력질주는 2~3초면 깨진다 (기획서 4-2 흔들림 게이지).
	const float Speed = GetVelocity().Size2D();
	if (Speed > WalkSpeed)
	{
		const float Ratio = Speed / WalkSpeed;
		EggWobble += DeltaSeconds * 0.15f * Ratio * Ratio;
	}
	else
	{
		EggWobble -= DeltaSeconds * (Speed > 10.f ? 0.15f : 0.3f);
	}
	EggWobble = FMath::Clamp(EggWobble, 0.f, 1.f);

	ASushiWildGameState* State = GetSushiState();
	if (EggWobble >= 1.f)
	{
		bCarryingEgg = false;
		CarriedEggMesh->SetVisibility(false);
		if (State)
		{
			State->PushToast(LOCTEXT("EggBroke", "와장창! 계란말이가 아니라 계란 쏟아짐…"), FLinearColor(1.f, 0.5f, 0.3f));
			State->RegisterMissionResult(SushiMissions::EggHeist, false);
		}
		return;
	}

	if (FVector::Dist2D(GetActorLocation(), SushiLayout::Counter) < SushiLayout::ShopDeliveryRadius)
	{
		if (Inventory.Num() >= BagSlots)
		{
			// 가방이 꽉 차면 알을 계속 들고 있는다 (자막은 3초에 한 번만).
			if (State && BagFullToastCooldown <= 0.f)
			{
				State->PushToast(LOCTEXT("BagFullEgg", "가방이 꽉 찼슈! 초밥부터 팔아야 알을 넣지유."), FLinearColor(1.f, 0.6f, 0.3f));
				BagFullToastCooldown = 3.f;
			}
		}
		else if (AddIngredient(CarriedEgg))
		{
			bCarryingEgg = false;
			CarriedEggMesh->SetVisibility(false);
			if (State)
			{
				State->PushToast(FText::Format(LOCTEXT("EggDelivered", "무사히 가게 도착! [{0}] 을(를) 냉장고에 넣었슈"), CarriedEgg.DisplayName), FLinearColor(0.6f, 1.f, 0.6f));
				State->RegisterMissionResult(SushiMissions::EggHeist, true);
			}
		}
	}
}

float ASushiChefCharacter::GetFreshness(const FSushiIngredient& Item) const
{
	const double ElapsedMinutes = (GetWorld()->GetTimeSeconds() - Item.AcquiredWorldSeconds) / 60.0;
	return static_cast<float>(SushiBalance::Freshness(ElapsedMinutes, Item.HalfLifeMinutes));
}

bool ASushiChefCharacter::AddIngredient(const FSushiIngredient& Item)
{
	if (Inventory.Num() >= BagSlots)
	{
		if (ASushiWildGameState* State = GetSushiState())
		{
			State->PushToast(LOCTEXT("BagFull", "가방이 꽉 찼슈! 초밥부터 팔아유."), FLinearColor(1.f, 0.6f, 0.3f));
		}
		return false;
	}
	Inventory.Add(Item);
	return true;
}

bool ASushiChefCharacter::TakeBestIngredient(FSushiIngredient& Out)
{
	int32 BestIndex = INDEX_NONE;
	float BestValue = -1.f;
	for (int32 i = 0; i < Inventory.Num(); ++i)
	{
		const float Value = Inventory[i].Quality * GetFreshness(Inventory[i]);
		if (Value > BestValue)
		{
			BestValue = Value;
			BestIndex = i;
		}
	}
	if (BestIndex == INDEX_NONE)
	{
		return false;
	}
	Out = Inventory[BestIndex];
	Inventory.RemoveAt(BestIndex);
	return true;
}

void ASushiChefCharacter::UpdateFocus()
{
	if (MinigameOwner)
	{
		Focused = MinigameOwner;
		return;
	}

	ASushiInteractable* Best = nullptr;
	float BestDistSq = TNumericLimits<float>::Max();
	const FVector Here = GetActorLocation();
	for (TActorIterator<ASushiInteractable> It(GetWorld()); It; ++It)
	{
		ASushiInteractable* Candidate = *It;
		const float DistSq = FVector::DistSquared2D(Here, Candidate->GetActorLocation());
		if (DistSq <= FMath::Square(Candidate->InteractRadius) && DistSq < BestDistSq && Candidate->CanInteract(this))
		{
			Best = Candidate;
			BestDistSq = DistSq;
		}
	}
	Focused = Best;
}

void ASushiChefCharacter::InteractStarted()
{
	if (bKnockedOut)
	{
		return;
	}
	PressedTarget = MinigameOwner ? MinigameOwner.Get() : Focused.Get();
	if (PressedTarget)
	{
		PressedTarget->OnInteractPressed(this);
	}
}

void ASushiChefCharacter::InteractCompleted()
{
	if (PressedTarget)
	{
		PressedTarget->OnInteractReleased(this);
		PressedTarget = nullptr;
	}
}

void ASushiChefCharacter::LockToMinigame(ASushiInteractable* Owner)
{
	MinigameOwner = Owner;
	GetCharacterMovement()->StopMovementImmediately();
}

void ASushiChefCharacter::UnlockMinigame(ASushiInteractable* Owner)
{
	if (MinigameOwner == Owner)
	{
		MinigameOwner = nullptr;
	}
}

void ASushiChefCharacter::ThrowCorn()
{
	if (MinigameOwner || bKnockedOut || !Controller)
	{
		return;
	}
	ASushiWildGameState* State = GetSushiState();
	if (CornCount <= 0)
	{
		if (State)
		{
			State->PushToast(LOCTEXT("NoCorn", "옥수수가 다 떨어졌슈. 가게에 가면 다시 챙겨유."));
		}
		return;
	}

	FRotator Aim = Controller->GetControlRotation();
	// 컨트롤 회전의 피치는 0~360 이라서 -180~180 으로 접은 뒤 살짝 위로 던진다
	Aim.Pitch = FMath::Clamp(FRotator::NormalizeAxis(Aim.Pitch) + 12.0, -10.0, 50.0);
	const FVector Start = GetActorLocation() + GetActorForwardVector() * 60.f + FVector(0.f, 0.f, 60.f);

	FActorSpawnParameters Params;
	Params.Owner = this;
	Params.Instigator = this;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (ACornBait* Corn = GetWorld()->SpawnActor<ACornBait>(ACornBait::StaticClass(), Start, Aim, Params))
	{
		Corn->Launch(Aim.Vector() * 1300.f);
		--CornCount;
	}
}

void ASushiChefCharacter::ApplyPeck(float Damage, AActor* Pecker)
{
	if (bKnockedOut)
	{
		return;
	}
	Health = FMath::Max(0.f, Health - Damage);
	if (bCarryingEgg)
	{
		EggWobble += 0.25f;
	}
	if (Pecker)
	{
		FVector Away = GetActorLocation() - Pecker->GetActorLocation();
		Away.Z = 0.f;
		LaunchCharacter(Away.GetSafeNormal() * 350.f + FVector(0.f, 0.f, 220.f), true, true);
	}

	ASushiWildGameState* State = GetSushiState();
	if (State && PeckToastCooldown <= 0.f)
	{
		State->PushToast(FText::Format(LOCTEXT("Pecked", "삐약! 쪼였슈 (체력 {0})"), FText::AsNumber(FMath::RoundToInt(Health))), FLinearColor(1.f, 0.7f, 0.4f), 1.5f);
		PeckToastCooldown = 1.f;
	}

	if (Health > 0.f)
	{
		return;
	}

	// 기절 → 펠리컨 구급대 (기획서 7-1 #1)
	bKnockedOut = true;
	MinigameOwner = nullptr;
	GetCharacterMovement()->StopMovementImmediately();
	if (State)
	{
		State->PushToast(LOCTEXT("KnockedOut", "기절! 머리 위에 별 세 개… 펠리컨 구급대 출동~"), FLinearColor(1.f, 0.4f, 0.4f), 4.f);
	}
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		PC->PlayerCameraManager->StartCameraFade(0.f, 1.f, 0.6f, FLinearColor::Black, false, true);
	}
	GetWorldTimerManager().SetTimer(KnockOutTimer, this, &ASushiChefCharacter::FinishKnockOut, 1.6f, false);
}

void ASushiChefCharacter::FinishKnockOut()
{
	ASushiWildGameState* State = GetSushiState();

	// 가방 재료 30% 분실
	const int32 ToLose = FMath::CeilToInt(Inventory.Num() * 0.3f);
	for (int32 i = 0; i < ToLose && Inventory.Num() > 0; ++i)
	{
		const int32 Index = State ? State->GetRng().RandRange(0, Inventory.Num() - 1) : 0;
		Inventory.RemoveAt(Index);
	}
	const bool bLostEgg = bCarryingEgg;
	bCarryingEgg = false;
	CarriedEggMesh->SetVisibility(false);

	TeleportTo(SushiLayout::ShopFront, FRotator(0.f, 0.f, 0.f));
	Health = MaxHealth * 0.6f;
	Stamina = 100.f;
	bKnockedOut = false;

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		PC->PlayerCameraManager->StartCameraFade(1.f, 0.f, 0.8f, FLinearColor::Black, false, false);
	}
	if (State)
	{
		State->PushToast(FText::Format(LOCTEXT("PelicanDrop", "펠리컨이 가게 앞에 '퉤' 뱉고 갔슈. 재료 {0}개 분실, 이마엔 닭 발자국 도장."),
			FText::AsNumber(ToLose)), FLinearColor(1.f, 0.8f, 0.5f), 5.f);
		if (bLostEgg)
		{
			State->RegisterMissionResult(SushiMissions::EggHeist, false);
		}
	}
}

void ASushiChefCharacter::CheckSea()
{
	const FVector Here = GetActorLocation();
	if (Here.Z < -1000.f)
	{
		// 섬 끝에서 떨어지면 펠리컨이 주워 온다
		if (ASushiWildGameState* State = GetSushiState())
		{
			State->PushToast(LOCTEXT("FellOff", "섬 밖으로 떨어졌는디 펠리컨이 주워 왔슈."), FLinearColor(0.5f, 0.8f, 1.f));
		}
		TeleportTo(SushiLayout::ShopFront, FRotator::ZeroRotator);
		return;
	}
	const bool bPastShore = Here.X > SushiLayout::ShoreX;
	const bool bOffPier = FMath::Abs(Here.Y) > SushiLayout::PierHalfWidth + 20.f || Here.X > SushiLayout::PierEndX + 40.f;
	if (!bPastShore || !bOffPier)
	{
		return;
	}

	if (ASushiWildGameState* State = GetSushiState())
	{
		State->PushToast(bCarryingEgg
			? LOCTEXT("SeaEgg", "풍덩! 갈매기들이 건져 줬는디… 알은 물고기 밥이 됐슈.")
			: LOCTEXT("Sea", "풍덩! 갈매기들이 건져서 해변에 던져 놨슈."), FLinearColor(0.5f, 0.8f, 1.f));
	}
	bCarryingEgg = false;
	CarriedEggMesh->SetVisibility(false);
	TeleportTo(SushiLayout::BeachShore, FRotator(0.f, 0.f, 0.f));
}

void ASushiChefCharacter::TeleportTo(const FVector& Location, const FRotator& Facing)
{
	SetActorLocation(Location, false, nullptr, ETeleportType::TeleportPhysics);
	SetActorRotation(Facing);
	if (Controller)
	{
		Controller->SetControlRotation(Facing);
	}
	GetCharacterMovement()->StopMovementImmediately();
}

ASushiWildGameState* ASushiChefCharacter::GetSushiState() const
{
	return GetWorld() ? GetWorld()->GetGameState<ASushiWildGameState>() : nullptr;
}

#undef LOCTEXT_NAMESPACE
