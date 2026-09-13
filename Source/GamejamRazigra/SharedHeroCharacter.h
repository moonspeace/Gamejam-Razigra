#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "SharedHeroCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UMaterialInstanceDynamic;
class UStaticMeshComponent;
class UTextBlock;
class UWidgetComponent;

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
    Look,
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
    void SetParticipantAbility(int32 ParticipantIndex, bool bPressed);

    UFUNCTION(BlueprintPure, Category="Razigra|Abilities")
    bool IsHealingActive() const { return bHealingActive; }

    UFUNCTION(BlueprintPure, Category="Razigra|Abilities")
    bool IsShieldActive() const { return bShieldActive; }

    /** Healing plants the hero; shielding deliberately allows normal movement. */
    UFUNCTION(BlueprintPure, Category="Razigra|Abilities")
    bool IsAbilityRooting() const { return bHealingActive; }

    UFUNCTION(BlueprintPure, Category="Razigra|Animation")
    bool IsCharacterCrouching() const { return bIsCrouched; }

    UFUNCTION(BlueprintPure, Category="Razigra|Combat")
    bool IsCrouchDamageImmunityActive() const;

    UFUNCTION(BlueprintPure, Category="Razigra|Animation")
    bool IsCharacterJumping() const;

    UFUNCTION(BlueprintPure, Category="Razigra|Animation")
    bool IsCharacterFalling() const;

    UFUNCTION(BlueprintPure, Category="Razigra|Animation")
    bool IsCharacterMoving() const;

    UFUNCTION(BlueprintPure, Category="Razigra|Animation")
    bool IsCharacterFiring() const { return bIsFiring; }

    /**
     * Degrees per second the hero swings round to face the aim yaw. High values feel snappy and
     * keep the crosshair and the body aligned; lower ones let the body trail the camera.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Razigra|Animation", meta=(ClampMin="30"))
    float TurnRateDegreesPerSecond = 500.0f;

    /** Horizontal speed: the value a locomotion blend space's Speed axis expects. */
    UFUNCTION(BlueprintPure, Category="Razigra|Animation")
    float GetGroundSpeed() const;

    /**
     * Angle between where the hero faces and where it is actually travelling, -180..180.
     * Computed exactly the way UKismetAnimationLibrary::CalculateDirection does, so the
     * Direction axis of a third-person blend space reads the same as on the template character.
     */
    UFUNCTION(BlueprintPure, Category="Razigra|Animation")
    float GetMovementDirection() const;

    /**
     * Whether locomotion should play. Speed-based on purpose: the template character tests
     * GetCurrentAcceleration(), but input acceleration is not replicated, and on a simulated
     * proxy character movement substitutes the normalised velocity for it. The shared hero is
     * a simulated proxy everywhere except the host, so speed is the one signal that means the
     * same thing on every machine.
     */
    UFUNCTION(BlueprintPure, Category="Razigra|Animation")
    bool ShouldMove() const;

    UFUNCTION(BlueprintPure, Category="Razigra|Animation")
    float GetAimPitch() const;

    UFUNCTION(BlueprintPure, Category="Razigra|Animation")
    float GetAimYaw() const;

    /**
     * Nothing ever possesses the shared hero, and an unpossessed pawn reports zero pitch here,
     * which leaves any aim-offset blend space stuck at its centre. Report the replicated
     * consensus aim instead. Aim offsets and weapon aiming read this; movement never does.
     */
    virtual FRotator GetBaseAimRotation() const override;

    /** Recoil bar, 0 to 1. Replicated so both players' HUDs can show the same meter. */
    UFUNCTION(BlueprintPure, Category="Razigra|Weapon")
    float GetRecoilHeat() const { return RecoilHeat; }

    /** True while the weapon is locked out after the bar filled. */
    UFUNCTION(BlueprintPure, Category="Razigra|Weapon")
    bool IsWeaponOverheated() const { return bWeaponOverheated; }

    UFUNCTION(BlueprintImplementableEvent, Category="Razigra|Weapon", meta=(DisplayName="On Weapon Overheated"))
    void BP_OnWeaponOverheated();

    UFUNCTION(BlueprintPure, Category="Razigra|State")
    bool IsDead() const { return bIsDead; }

    UFUNCTION(BlueprintPure, Category="Razigra|State")
    float GetHealth() const { return Health; }

    UFUNCTION(BlueprintPure, Category="Razigra|State")
    float GetHealthNormalized() const;

    /** Bit mask of the actions a participant is currently holding, replicated for the HUD. */
    UFUNCTION(BlueprintPure, Category="Razigra|Consensus")
    int32 GetParticipantActionMask(int32 ParticipantIndex) const;

    UFUNCTION(BlueprintPure, Category="Razigra|Consensus")
    bool IsActionPressedBy(int32 ParticipantIndex, EConsensusAction Action) const;

    UFUNCTION(BlueprintPure, Category="Razigra|Consensus")
    int32 GetRequiredConsensusParticipants() const { return RequiredConsensusParticipants; }

    /** That participant's current mouse delta, replicated so the HUD can meter both players. */
    UFUNCTION(BlueprintPure, Category="Razigra|Consensus")
    FVector2D GetParticipantLookAxis(int32 ParticipantIndex) const;

    /** Where shots visually originate: the weapon muzzle socket, or the camera if it is absent. */
    UFUNCTION(BlueprintPure, Category="Razigra|Weapon")
    FVector GetMuzzleLocation() const;

    /**
     * Override in a Blueprint child of this class to add muzzle flashes, tracers, sounds,
     * camera shakes, decals and so on. Runs on every machine, server and clients alike.
     */
    UFUNCTION(BlueprintImplementableEvent, Category="Razigra|Weapon", meta=(DisplayName="On Gun Fired"))
    void BP_OnGunFired(const FVector& MuzzleLocation, const FVector& ImpactPoint, bool bHit, AActor* HitActor);

    UFUNCTION(BlueprintImplementableEvent, Category="Razigra|State", meta=(DisplayName="On Hero Died"))
    void BP_OnHeroDied();

    UFUNCTION(BlueprintImplementableEvent, Category="Razigra|State", meta=(DisplayName="On Hero Damaged"))
    void BP_OnHeroDamaged(float DamageAmount);

    UFUNCTION(BlueprintImplementableEvent, Category="Razigra|Abilities", meta=(DisplayName="On Healing State Changed"))
    void BP_OnHealingStateChanged(bool bActive);

    UFUNCTION(BlueprintImplementableEvent, Category="Razigra|Abilities", meta=(DisplayName="On Shield State Changed"))
    void BP_OnShieldStateChanged(bool bActive);

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Camera")
    TObjectPtr<USpringArmComponent> CameraBoom;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Camera")
    TObjectPtr<UCameraComponent> FollowCamera;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Abilities")
    TObjectPtr<UStaticMeshComponent> ShieldMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Abilities")
    TArray<TObjectPtr<UStaticMeshComponent>> HealingEffectMeshes;

protected:
    UFUNCTION()
    void OnRep_AimRotation();

    UFUNCTION()
    void OnRep_AbilityState();

    UFUNCTION(NetMulticast, Unreliable)
    void MulticastGunFired(const FVector_NetQuantize& MuzzleLocation, const FVector_NetQuantize& ImpactPoint,
        bool bHit, AActor* HitActor);

    UFUNCTION(NetMulticast, Reliable)
    void MulticastHeroDied();

    UFUNCTION(NetMulticast, Reliable)
    void MulticastWeaponOverheated();

    UFUNCTION(NetMulticast, Reliable)
    void MulticastHeroDamaged(float DamageAmount);

private:
    static constexpr int32 ParticipantCount = 2;
    static constexpr int32 ActionCount = static_cast<int32>(EConsensusAction::MAX);

    bool ParticipantActions[ParticipantCount][ActionCount] = {};
    FVector2D PendingLook[ParticipantCount] = {};
    double LookReceivedAt[ParticipantCount] = {};
    double LookActiveUntil[ParticipantCount] = {};
    bool bLookPending[ParticipantCount] = {};
    bool bWasJumpConsensus = false;
    double NextFireTime = 0.0;
    double FiringVisualUntil = 0.0;
    float AbilityVisualTime = 0.0f;

    UPROPERTY(Replicated)
    int32 RequiredConsensusParticipants = 2;

    /** Replicated mirrors of ParticipantActions so both clients can draw the consensus HUD. */
    UPROPERTY(Replicated)
    int32 PlayerOneActionMask = 0;

    UPROPERTY(Replicated)
    int32 PlayerTwoActionMask = 0;

    UPROPERTY(Replicated)
    FVector2D PlayerOneLookAxis = FVector2D::ZeroVector;

    UPROPERTY(Replicated)
    FVector2D PlayerTwoLookAxis = FVector2D::ZeroVector;

    UPROPERTY(Replicated)
    float RecoilHeat = 0.0f;

    UPROPERTY(Replicated)
    bool bWeaponOverheated = false;

    double OverheatUntil = 0.0;
    double CrouchDamageImmunityUntil = 0.0;
    bool bWasCrouchConsensus = false;

    UPROPERTY(ReplicatedUsing=OnRep_AimRotation)
    FRotator AimRotation;

    UPROPERTY(Replicated)
    float Health = 100.0f;

    UPROPERTY(Replicated)
    bool bIsFiring = false;

    UPROPERTY(Replicated)
    bool bIsDead = false;

    UPROPERTY(ReplicatedUsing=OnRep_AbilityState)
    bool bHealingActive = false;

    UPROPERTY(ReplicatedUsing=OnRep_AbilityState)
    bool bShieldActive = false;

    /** Replicated presentation flag for the short damage-immunity window started by crouching. */
    UPROPERTY(ReplicatedUsing=OnRep_AbilityState)
    bool bCrouchImmunityVisualActive = false;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> ShieldDynamicMaterial;

    /** Local presentation state; gameplay protection continues to use replicated bShieldActive. */
    float ShieldVisualAlpha = 0.0f;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> HealingDynamicMaterial;

    /** Restored when the shield drops; avoids hard-coding the hero Blueprint's collision setup. */
    TEnumAsByte<ECollisionResponse> DefaultPawnCollisionResponse = ECR_Block;

    bool HasConsensus(EConsensusAction Action) const;
    void EnsureVisibleMesh();
    void ConfigureCamera();
    void RefreshActionMasks();
    void ProcessMovement();
    void ProcessLook();
    void ProcessActions();
    void FireGun();
    void UpdateRecoilHeat(float DeltaSeconds);
    void UpdateShieldVisual(float DeltaSeconds);
    void TriggerWeaponOverheat();
    void ConfigureAbilityVisuals();
};
