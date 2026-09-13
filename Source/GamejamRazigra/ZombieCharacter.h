#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ZombieCharacter.generated.h"

class ASharedHeroCharacter;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class USkeletalMeshComponent;

/** Simple server-controlled zombie: path to the shared hero, then attack. */
UCLASS(Blueprintable)
class GAMEJAMRAZIGRA_API AZombieCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    AZombieCharacter();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent,
        AController* EventInstigator, AActor* DamageCauser) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UFUNCTION(BlueprintPure, Category="Razigra|Zombie")
    bool IsAttacking() const { return bIsAttacking; }

    UFUNCTION(BlueprintPure, Category="Razigra|Zombie")
    bool IsDead() const { return bIsDead; }

    UFUNCTION(BlueprintPure, Category="Razigra|Zombie")
    float GetHealth() const { return Health; }

    UFUNCTION(BlueprintPure, Category="Razigra|Zombie")
    float GetMovementSpeed() const { return GetVelocity().Size2D(); }

    UFUNCTION(BlueprintImplementableEvent, Category="Razigra|Zombie", meta=(DisplayName="On Zombie Attack"))
    void BP_OnZombieAttack();

    UFUNCTION(BlueprintImplementableEvent, Category="Razigra|Zombie", meta=(DisplayName="On Zombie Died"))
    void BP_OnZombieDied();

    UFUNCTION(BlueprintImplementableEvent, Category="Razigra|Zombie", meta=(DisplayName="On Zombie Damaged"))
    void BP_OnZombieDamaged(float DamageAmount);

    /** Fires after the wind-up, whether the hero remained in range or successfully dodged. */
    UFUNCTION(BlueprintImplementableEvent, Category="Razigra|Zombie", meta=(DisplayName="On Zombie Attack Resolved"))
    void BP_OnZombieAttackResolved(bool bHitHero);

protected:
    UFUNCTION()
    void OnRep_ClothingVariations();

    UFUNCTION(NetMulticast, Unreliable)
    void MulticastAttack();

    UFUNCTION(NetMulticast, Reliable)
    void MulticastDied();

    UFUNCTION(NetMulticast, Reliable)
    void MulticastDamageReceived(float DamageAmount, bool bLethalHit);

    UFUNCTION(NetMulticast, Unreliable)
    void MulticastAttackResolved(bool bHitHero);

private:
    UPROPERTY(Replicated)
    float Health = 50.0f;

    UPROPERTY(Replicated)
    bool bIsAttacking = false;

    UPROPERTY(Replicated)
    bool bIsDead = false;

    UPROPERTY(ReplicatedUsing=OnRep_ClothingVariations)
    TArray<int32> ClothingVariationIndices;

    UPROPERTY(Transient)
    TArray<TObjectPtr<USkeletalMeshComponent>> ClothingComponents;

    UPROPERTY()
    TObjectPtr<ASharedHeroCharacter> TargetHero;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UMaterialInstanceDynamic>> SpawnMaterials;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UMaterialInterface>> PreSpawnMaterials;

    UPROPERTY(Transient)
    TArray<TObjectPtr<USkeletalMeshComponent>> PreSpawnMaterialComponents;

    TArray<int32> PreSpawnMaterialSlots;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UMaterialInstanceDynamic>> DeathMaterials;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UMaterialInterface>> PreHitMaterials;

    double NextPathRefreshTime = 0.0;
    double NextAttackTime = 0.0;
    double AttackHitTime = 0.0;
    float DeathEffectElapsed = 0.0f;
    float SpawnEffectElapsed = 0.0f;
    bool bSpawnEffectActive = false;
    float TacticalAngleRadians = 0.0f;
    double LastProgressCheckTime = 0.0;
    FVector LastProgressLocation = FVector::ZeroVector;
    FVector LastTrackedHeroLocation = FVector::ZeroVector;
    FTimerHandle RestoreAnimationTimer;
    FTimerHandle HitFlashTimer;

    void AcquireTarget();
    void ApplyClothingVariations();
    TArray<USkeletalMeshComponent*> GetVisualMeshes() const;
    void FaceTarget();
    void UpdateServerBehavior();
    void StartAttack();
    void ResolveAttack();
    void StartDeathEffect();
    void UpdateDeathEffect(float DeltaSeconds);
    void StartSpawnEffect();
    void UpdateSpawnEffect(float DeltaSeconds);
    void FinishSpawnEffect();
    void RestoreAnimationBlueprint();
    void StartHitEffect();
    void RestoreHitEffect();
    FVector CalculateTacticalGoal(bool bRecovering) const;
};
