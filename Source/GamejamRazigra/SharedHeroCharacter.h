#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "SharedHeroCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;

UENUM(BlueprintType)
enum class EConsensusAction : uint8
{
    MoveForward,
    MoveBackward,
    MoveLeft,
    MoveRight,
    Jump,
    Crouch,
    Fire,
    MAX UMETA(Hidden)
};

/** Server-authoritative pawn driven by the overlapping input of both players. */
UCLASS(Blueprintable)
class GAMEJAMRAZIGRA_API ASharedHeroCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    ASharedHeroCharacter();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent,
        AController* EventInstigator, AActor* DamageCauser) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    void SetParticipantAction(int32 ParticipantIndex, EConsensusAction Action, bool bPressed);
    void SubmitParticipantLook(int32 ParticipantIndex, const FVector2D& LookDelta);
    void ResetParticipant(int32 ParticipantIndex);
    void SetRequiredConsensusParticipants(int32 NewRequiredCount);

    UFUNCTION(BlueprintPure, Category="Razigra|Animation")
    bool IsCharacterCrouching() const { return bIsCrouched; }

    UFUNCTION(BlueprintPure, Category="Razigra|Animation")
    bool IsCharacterJumping() const;

    UFUNCTION(BlueprintPure, Category="Razigra|Animation")
    bool IsCharacterFalling() const;

    UFUNCTION(BlueprintPure, Category="Razigra|Animation")
    bool IsCharacterMoving() const;

    UFUNCTION(BlueprintPure, Category="Razigra|Animation")
    bool IsCharacterFiring() const { return bIsFiring; }

    UFUNCTION(BlueprintPure, Category="Razigra|State")
    bool IsDead() const { return bIsDead; }

    UFUNCTION(BlueprintPure, Category="Razigra|State")
    float GetHealth() const { return Health; }

    UFUNCTION(BlueprintPure, Category="Razigra|State")
    float GetHealthNormalized() const;

    UFUNCTION(BlueprintImplementableEvent, Category="Razigra|Weapon", meta=(DisplayName="On Gun Fired"))
    void BP_OnGunFired(const FVector& TraceStart, const FVector& TraceEnd, bool bHit);

    UFUNCTION(BlueprintImplementableEvent, Category="Razigra|State", meta=(DisplayName="On Hero Died"))
    void BP_OnHeroDied();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Camera")
    TObjectPtr<USpringArmComponent> CameraBoom;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Camera")
    TObjectPtr<UCameraComponent> FollowCamera;

protected:
    UFUNCTION()
    void OnRep_AimRotation();

    UFUNCTION(NetMulticast, Unreliable)
    void MulticastGunFired(const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& TraceEnd, bool bHit);

    UFUNCTION(NetMulticast, Reliable)
    void MulticastHeroDied();

private:
    static constexpr int32 ParticipantCount = 2;
    static constexpr int32 ActionCount = static_cast<int32>(EConsensusAction::MAX);

    bool ParticipantActions[ParticipantCount][ActionCount] = {};
    FVector2D PendingLook[ParticipantCount] = {};
    double LookReceivedAt[ParticipantCount] = {};
    bool bLookPending[ParticipantCount] = {};
    bool bWasJumpConsensus = false;
    int32 RequiredConsensusParticipants = ParticipantCount;
    double NextFireTime = 0.0;
    double FiringVisualUntil = 0.0;

    UPROPERTY(ReplicatedUsing=OnRep_AimRotation)
    FRotator AimRotation;

    UPROPERTY(Replicated)
    float Health = 100.0f;

    UPROPERTY(Replicated)
    bool bIsFiring = false;

    UPROPERTY(Replicated)
    bool bIsDead = false;

    bool HasConsensus(EConsensusAction Action) const;
    void ProcessMovement();
    void ProcessLook();
    void ProcessActions();
    void FireGun();
};
