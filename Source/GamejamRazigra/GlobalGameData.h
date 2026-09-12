#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GlobalGameData.generated.h"

class ASharedHeroCharacter;
class AZombieCharacter;
class UAnimInstance;
class UCoopHudWidget;
class UCoopMenuWidget;

/**
 * The one gameplay-data object for Razigra. Create Blueprint children when a
 * designer needs computed/helper behavior, and keep the instance at
 * /Game/Data/GlobalGameData.
 */
UCLASS(BlueprintType, Blueprintable)
class GAMEJAMRAZIGRA_API UGlobalGameData : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UGlobalGameData();

    UFUNCTION(BlueprintPure, Category="Razigra|Data", meta=(WorldContext="WorldContextObject"))
    static const UGlobalGameData* Get(const UObject* WorldContextObject);

    /**
     * The hero Blueprint the shared character is spawned from. Configure the mesh,
     * animation Blueprint, and every other visual on that Blueprint instead of here.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Hero")
    TSoftClassPtr<ASharedHeroCharacter> HeroBlueprint;

    /** Resolves HeroBlueprint, falling back to the C++ class when it is unset. */
    UFUNCTION(BlueprintPure, Category="Hero")
    TSubclassOf<ASharedHeroCharacter> GetHeroClass() const;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Classes")
    TSubclassOf<AZombieCharacter> ZombieClass;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Classes")
    TSubclassOf<UCoopMenuWidget> MainMenuWidgetClass;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Classes")
    TSubclassOf<UCoopHudWidget> GameplayHudWidgetClass;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Maps", meta=(AllowedClasses="/Script/Engine.World"))
    TSoftObjectPtr<UWorld> GameplayMap;

    /** Seconds the screen takes to fade up from black once the match starts. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Presentation", meta=(ClampMin="0"))
    float GameplayFadeInSeconds = 0.9f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Presentation")
    TSoftObjectPtr<USkeletalMesh> ZombieMesh;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Presentation")
    TSoftClassPtr<UAnimInstance> ZombieAnimationClass;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Hero", meta=(ClampMin="0"))
    float HeroMaxHealth = 100.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Hero", meta=(ClampMin="0"))
    float WalkSpeed = 500.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Hero", meta=(ClampMin="0"))
    float CrouchedSpeed = 250.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Hero", meta=(ClampMin="0"))
    float JumpVelocity = 600.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Hero", meta=(ClampMin="0"))
    float LookSensitivity = 1.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Hero", meta=(ClampMin="0.01"))
    float LookInputGraceSeconds = 0.12f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon", meta=(ClampMin="0"))
    float FireDamage = 25.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon", meta=(ClampMin="0"))
    float FireRange = 10000.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon", meta=(ClampMin="0.01"))
    float FireInterval = 0.2f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon")
    FName MuzzleSocketName = TEXT("Muzzle");

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Zombie", meta=(ClampMin="0"))
    float ZombieMaxHealth = 50.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Zombie", meta=(ClampMin="0"))
    float ZombieMoveSpeed = 250.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Zombie", meta=(ClampMin="0"))
    float ZombieAttackRange = 140.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Zombie", meta=(ClampMin="0"))
    float ZombieAttackDamage = 10.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Zombie", meta=(ClampMin="0.01"))
    float ZombieAttackInterval = 1.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Zombie", meta=(ClampMin="0.01"))
    float ZombiePathRefreshInterval = 0.35f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Network", meta=(ClampMin="1", ClampMax="2"))
    int32 RequiredPlayers = 2;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Network")
    FString SessionName = TEXT("Zombie Zero Two Player Co-op");

    /**
     * When true the shared hero is not spawned until every required player has connected, so
     * nobody starts playing alone. The host still opens the listen map as soon as the session
     * is created; only the start of play waits. Turn it off to begin the moment the map loads.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Network")
    bool bWaitForAllPlayersBeforeStart = true;
};
