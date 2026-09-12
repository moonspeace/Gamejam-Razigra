#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ZombieSpawner.generated.h"

class AZombieCharacter;
class UArrowComponent;
class USceneComponent;
class USphereComponent;

/**
 * Drop one of these in the level wherever zombies should come from. It carries a root and a
 * wireframe sphere so it can be selected and moved in the viewport, and the sphere shows the
 * area zombies will appear in.
 */
UCLASS(Blueprintable)
class GAMEJAMRAZIGRA_API AZombieSpawner : public AActor
{
    GENERATED_BODY()

public:
    AZombieSpawner();

    virtual void BeginPlay() override;
    virtual void OnConstruction(const FTransform& Transform) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Spawner")
    TObjectPtr<USceneComponent> SceneRoot;

    /** Wireframe preview of SpawnRadius. Editor only: never rendered or collided with in game. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Spawner")
    TObjectPtr<USphereComponent> SpawnArea;

    UFUNCTION(BlueprintCallable, Category="Razigra|Spawner", BlueprintAuthorityOnly)
    void SetSpawnerActive(bool bNewActive);

    UFUNCTION(BlueprintCallable, Category="Razigra|Spawner", BlueprintAuthorityOnly)
    void SpawnZombieNow();

    UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_Active, Category="Spawner")
    bool bSpawnerActive = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Spawner", meta=(ClampMin="0.01"))
    float SpawnRate = 4.0f;

    /** Zombies appear at a random navigable point within this radius. Zero spawns them right here. */
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
#if WITH_EDITORONLY_DATA
    UPROPERTY()
    TObjectPtr<UArrowComponent> DirectionArrow;
#endif

    FTimerHandle SpawnTimer;

    UPROPERTY()
    TArray<TObjectPtr<AZombieCharacter>> SpawnedZombies;

    void RefreshTimer();
};
