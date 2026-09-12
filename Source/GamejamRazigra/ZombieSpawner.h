#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ZombieSpawner.generated.h"

class AZombieCharacter;

UCLASS(Blueprintable)
class GAMEJAMRAZIGRA_API AZombieSpawner : public AActor
{
    GENERATED_BODY()

public:
    AZombieSpawner();

    virtual void BeginPlay() override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UFUNCTION(BlueprintCallable, Category="Razigra|Spawner", BlueprintAuthorityOnly)
    void SetSpawnerActive(bool bNewActive);

    UFUNCTION(BlueprintCallable, Category="Razigra|Spawner", BlueprintAuthorityOnly)
    void SpawnZombieNow();

    UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_Active, Category="Spawner")
    bool bSpawnerActive = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Spawner", meta=(ClampMin="0.01"))
    float SpawnRate = 4.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Spawner", meta=(ClampMin="0"))
    float SpawnRadius = 400.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Spawner", meta=(ClampMin="0"))
    int32 MaxAliveZombies = 20;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Spawner")
    TSubclassOf<AZombieCharacter> ZombieClass;

    UFUNCTION(BlueprintImplementableEvent, Category="Razigra|Spawner", meta=(DisplayName="On Spawner Active Changed"))
    void BP_OnSpawnerActiveChanged(bool bIsActive);

protected:
    UFUNCTION()
    void OnRep_Active();

private:
    FTimerHandle SpawnTimer;

    UPROPERTY()
    TArray<TObjectPtr<AZombieCharacter>> SpawnedZombies;

    void RefreshTimer();
};
