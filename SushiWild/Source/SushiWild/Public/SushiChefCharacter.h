#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "SushiInteractable.h"
#include "SushiTypes.h"
#include "SushiChefCharacter.generated.h"

class ASushiWildGameState;
class UCameraComponent;
class UInputAction;
class UInputMappingContext;
class USpringArmComponent;
class UStaticMeshComponent;
struct FInputActionValue;

// 주인공 '샤리'. 자기 몸보다 큰 식칼을 등에 메고 다닌다.
UCLASS()
class SUSHIWILD_API ASushiChefCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ASushiChefCharacter();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void PawnClientRestart() override;
	virtual void OnJumped_Implementation() override;
	virtual void OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;
	virtual void OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;

	// ── 능력치 (기획서 4-1) ──
	UPROPERTY(EditAnywhere, Category = "Sushi|Stats") float MaxHealth = 100.f;
	UPROPERTY(EditAnywhere, Category = "Sushi|Stats") float WalkSpeed = 450.f;
	UPROPERTY(EditAnywhere, Category = "Sushi|Stats") float SprintSpeed = 850.f;
	UPROPERTY(EditAnywhere, Category = "Sushi|Stats") int32 BagSlots = 6;

	float GetHealth() const { return Health; }
	float GetStamina() const { return Stamina; }
	// 0~1. 경비 동물이 듣는 발소리 크기 (날씨 보정 전).
	float GetNoise() const { return Noise; }
	// 0~1. 경비가 나를 얼마나 잘 보는지 (웅크리면 절반).
	float GetVisibilityFactor() const;

	// ── 가방 ──
	const TArray<FSushiIngredient>& GetInventory() const { return Inventory; }
	float GetFreshness(const FSushiIngredient& Item) const;
	bool AddIngredient(const FSushiIngredient& Item);
	// 신선도 × 품질이 가장 좋은 재료를 꺼낸다. 없으면 false.
	bool TakeBestIngredient(FSushiIngredient& Out);

	// ── 알 운반 (기획서 4-2 잠입: 뛰면 흔들림 게이지가 차서 깨진다) ──
	bool IsCarryingEgg() const { return bCarryingEgg; }
	float GetEggWobble() const { return EggWobble; }
	void StartCarryingEgg(const FSushiIngredient& Egg);

	// ── 피해와 기절 (기획서 7-1 #1 펠리컨 구급대) ──
	void ApplyPeck(float Damage, AActor* Pecker);
	bool IsKnockedOut() const { return bKnockedOut; }

	// ── 상호작용 / 미니게임 잠금 ──
	ASushiInteractable* GetFocusedInteractable() const { return Focused; }
	ASushiInteractable* GetMinigameOwner() const { return MinigameOwner; }
	void LockToMinigame(ASushiInteractable* Owner);
	void UnlockMinigame(ASushiInteractable* Owner);
	void TeleportTo(const FVector& Location, const FRotator& Facing);
	int32 GetCornCount() const { return CornCount; }

protected:
	UPROPERTY(VisibleAnywhere) TObjectPtr<USpringArmComponent> CameraBoom;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UCameraComponent> FollowCamera;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> VisualRoot;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> CarriedEggMesh;

	UPROPERTY() TObjectPtr<UInputMappingContext> MappingContext;
	UPROPERTY() TObjectPtr<UInputAction> MoveAction;
	UPROPERTY() TObjectPtr<UInputAction> LookAction;
	UPROPERTY() TObjectPtr<UInputAction> JumpAction;
	UPROPERTY() TObjectPtr<UInputAction> SprintAction;
	UPROPERTY() TObjectPtr<UInputAction> CrouchAction;
	UPROPERTY() TObjectPtr<UInputAction> InteractAction;
	UPROPERTY() TObjectPtr<UInputAction> ThrowAction;

private:
	void BuildInput();
	void BuildVisuals();

	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void StartJump();
	void SprintStarted();
	void SprintStopped();
	void ToggleCrouch();
	void InteractStarted();
	void InteractCompleted();
	void ThrowCorn();

	void UpdateFocus();
	void UpdateNoiseAndStamina(float DeltaSeconds);
	void UpdateEgg(float DeltaSeconds);
	void CheckSea();
	void FinishKnockOut();

	ASushiWildGameState* GetSushiState() const;

	UPROPERTY() TArray<FSushiIngredient> Inventory;
	UPROPERTY() FSushiIngredient CarriedEgg;
	UPROPERTY() TObjectPtr<ASushiInteractable> Focused;
	UPROPERTY() TObjectPtr<ASushiInteractable> MinigameOwner;
	UPROPERTY() TObjectPtr<ASushiInteractable> PressedTarget;

	float Health = 100.f;
	float Stamina = 100.f;
	float Noise = 0.f;
	float EggWobble = 0.f;
	float PeckToastCooldown = 0.f;
	float BagFullToastCooldown = 0.f;
	int32 CornCount = 5;
	bool bCarryingEgg = false;
	bool bWantsSprint = false;
	bool bKnockedOut = false;

	FTimerHandle KnockOutTimer;
};
