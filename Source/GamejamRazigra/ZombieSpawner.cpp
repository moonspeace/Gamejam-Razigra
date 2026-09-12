#include "ZombieSpawner.h"

#include "Components/ArrowComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
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

    // Without a root component the actor has no transform to place, and nothing to click on in
    // the viewport. The sphere doubles as the spawn-radius preview.
    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);

    SpawnArea = CreateDefaultSubobject<USphereComponent>(TEXT("SpawnArea"));
    SpawnArea->SetupAttachment(SceneRoot);
    SpawnArea->SetSphereRadius(SpawnRadius);
    SpawnArea->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    SpawnArea->SetCollisionResponseToAllChannels(ECR_Ignore);
    SpawnArea->SetGenerateOverlapEvents(false);
    SpawnArea->SetHiddenInGame(true);
    SpawnArea->ShapeColor = FColor(190, 60, 45);

#if WITH_EDITORONLY_DATA
    DirectionArrow = CreateEditorOnlyDefaultSubobject<UArrowComponent>(TEXT("DirectionArrow"));
    if (DirectionArrow)
    {
        DirectionArrow->SetupAttachment(SceneRoot);
        DirectionArrow->ArrowColor = FColor(190, 60, 45);
        DirectionArrow->bIsScreenSizeScaled = true;
    }
#endif
}

void AZombieSpawner::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    if (SpawnArea)
    {
        SpawnArea->SetSphereRadius(FMath::Max(SpawnRadius, 1.0f), false);
    }
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

    if (TotalSpawnLimit > 0 && TotalSpawned >= TotalSpawnLimit)
    {
        GetWorldTimerManager().ClearTimer(SpawnTimer);
        return;
    }

    SpawnedZombies.RemoveAll([](const TObjectPtr<AZombieCharacter>& Zombie) { return !IsValid(Zombie); });
    if (SpawnedZombies.Num() >= MaxAliveZombies || !ZombieClass)
    {
        return;
    }

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    // Spawn where this actor was placed. A radius scatters them around it; zero pins them to it.
    const FVector Origin = GetActorLocation();
    FVector SpawnLocation = Origin + FVector(0.0f, 0.0f, 100.0f);
    FNavLocation NavLocation;
    if (UNavigationSystemV1* Navigation = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
    {
        const bool bFound = SpawnRadius > 0.0f
            ? Navigation->GetRandomReachablePointInRadius(Origin, SpawnRadius, NavLocation)
            : Navigation->ProjectPointToNavigation(Origin, NavLocation, FVector(0.0f, 0.0f, 200.0f));
        if (bFound)
        {
            SpawnLocation = NavLocation.Location + FVector(0.0f, 0.0f, 100.0f);
        }
    }
    if (AZombieCharacter* Zombie = GetWorld()->SpawnActor<AZombieCharacter>(ZombieClass,
        SpawnLocation, GetActorRotation(), Params))
    {
        SpawnedZombies.Add(Zombie);
        ++TotalSpawned;
        if (TotalSpawnLimit > 0 && TotalSpawned >= TotalSpawnLimit)
        {
            GetWorldTimerManager().ClearTimer(SpawnTimer);
        }
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
