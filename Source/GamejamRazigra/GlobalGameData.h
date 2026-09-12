#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GlobalGameData.generated.h"

class ASharedHeroCharacter;
class AZombieCharacter;
class UAnimInstance;
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

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Classes")
    TSubclassOf<ASharedHeroCharacter> HeroClass;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Classes")
    TSubclassOf<AZombieCharacter> ZombieClass;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Classes")
    TSubclassOf<UCoopMenuWidget> MainMenuWidgetClass;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Maps", meta=(AllowedClasses="/Script/Engine.World"))
    TSoftObjectPtr<UWorld> GameplayMap;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Presentation")
    TSoftObjectPtr<USkeletalMesh> HeroMesh;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Presentation")
    TSoftClassPtr<UAnimInstance> HeroAnimationClass;

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

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Network")
    int32 RequiredPlayers = 2;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Network")
    FString SessionName = TEXT("Zombie Zero Two Player Co-op");
};
