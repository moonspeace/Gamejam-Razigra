#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ZombieCharacter.generated.h"

class ASharedHeroCharacter;

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

protected:
    UFUNCTION(NetMulticast, Unreliable)
    void MulticastAttack();

    UFUNCTION(NetMulticast, Reliable)
    void MulticastDied();

private:
    UPROPERTY(Replicated)
    float Health = 50.0f;

    UPROPERTY(Replicated)
    bool bIsAttacking = false;

    UPROPERTY(Replicated)
    bool bIsDead = false;

    UPROPERTY()
    TObjectPtr<ASharedHeroCharacter> TargetHero;

    double NextPathRefreshTime = 0.0;
    double NextAttackTime = 0.0;
    double AttackVisualUntil = 0.0;

    void AcquireTarget();
    void UpdateServerBehavior();
};
