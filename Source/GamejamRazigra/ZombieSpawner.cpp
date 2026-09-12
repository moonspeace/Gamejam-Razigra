#include "ZombieSpawner.h"

#include "Engine/World.h"
#include "NavigationSystem.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "ZombieCharacter.h"

AZombieSpawner::AZombieSpawner()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;
    ZombieClass = AZombieCharacter::StaticClass();
}

void AZombieSpawner::BeginPlay()
{
    Super::BeginPlay();
    if (HasAuthority())
    {
        RefreshTimer();
    }
}

void AZombieSpawner::SetSpawnerActive(bool bNewActive)
{
    if (!HasAuthority())
    {
        return;
    }
    bSpawnerActive = bNewActive;
    RefreshTimer();
    OnRep_Active();
}

void AZombieSpawner::RefreshTimer()
{
    GetWorldTimerManager().ClearTimer(SpawnTimer);
    if (!bSpawnerActive)
    {
        return;
    }
    const float Rate = FMath::Max(SpawnRate, 0.01f);
    GetWorldTimerManager().SetTimer(SpawnTimer, this, &ThisClass::SpawnZombieNow, Rate, true, Rate);
}

void AZombieSpawner::SpawnZombieNow()
{
    if (!HasAuthority() || !bSpawnerActive)
    {
        return;
    }

    SpawnedZombies.RemoveAll([](const TObjectPtr<AZombieCharacter>& Zombie) { return !IsValid(Zombie); });
    if (SpawnedZombies.Num() >= MaxAliveZombies || !ZombieClass)
    {
        return;
    }

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
    FVector SpawnLocation = GetActorLocation() + FVector(0.0f, 0.0f, 100.0f);
    FNavLocation NavLocation;
    if (UNavigationSystemV1* Navigation = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
    {
        if (Navigation->GetRandomReachablePointInRadius(GetActorLocation(), SpawnRadius, NavLocation))
        {
            SpawnLocation = NavLocation.Location + FVector(0.0f, 0.0f, 100.0f);
        }
    }
    if (AZombieCharacter* Zombie = GetWorld()->SpawnActor<AZombieCharacter>(ZombieClass,
        SpawnLocation, GetActorRotation(), Params))
    {
        SpawnedZombies.Add(Zombie);
    }
}

void AZombieSpawner::OnRep_Active()
{
    BP_OnSpawnerActiveChanged(bSpawnerActive);
}

void AZombieSpawner::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AZombieSpawner, bSpawnerActive);
}
