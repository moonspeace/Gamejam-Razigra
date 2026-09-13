#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Fonts/SlateFontInfo.h"
#include "GlobalGameData.generated.h"

class ASharedHeroCharacter;
class AZombieCharacter;
class UAnimInstance;
class UAnimMontage;
class UAnimSequenceBase;
class UCameraShakeBase;
class UCoopHudWidget;
class UCoopMenuWidget;
class UFont;
class UMaterialInterface;

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

    /** Camera boom offset. Used as socket offset normally, or relative offset when attached to a bone. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Camera")
    FVector CameraOffset = FVector(0.0f, 55.0f, 70.0f);

    /** Optional hero bone/socket for the camera boom. Leave None to retain root-mounted camera behavior. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Camera")
    FName CameraAttachBoneName = NAME_None;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Presentation")
    TSoftObjectPtr<USkeletalMesh> ZombieMesh;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Presentation")
    TSoftClassPtr<UAnimInstance> ZombieAnimationClass;

    /** Montage played on every machine when a zombie begins its attack wind-up. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Zombie|Animation")
    TSoftObjectPtr<UAnimMontage> ZombieAttackMontage;

    /** Direct attack clip used to guarantee playback even when the AnimBP has no montage slot. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Zombie|Animation")
    TSoftObjectPtr<UAnimSequenceBase> ZombieAttackAnimation;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Hero", meta=(ClampMin="0"))
    float HeroMaxHealth = 100.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Hero|Abilities", meta=(ClampMin="0"))
    float HealingPerSecond = 18.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Hero|Abilities")
    FLinearColor HealingEffectColor = FLinearColor(0.02f, 1.0f, 0.08f, 1.0f);

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Hero|Abilities", meta=(ClampMin="0"))
    float HealingEffectIntensity = 8.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Hero|Abilities")
    TSoftObjectPtr<UMaterialInterface> HealingEffectMaterial;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Hero|Abilities")
    FVector HealingEffectOffset = FVector(0.0f, 0.0f, 145.0f);

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Hero|Abilities", meta=(ClampMin="0.1"))
    float HealingPlusSize = 18.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Hero|Abilities")
    TSoftObjectPtr<UMaterialInterface> ShieldMaterial;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Hero|Abilities", meta=(ClampMin="50"))
    float ShieldRadius = 135.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Hero|Abilities")
    FLinearColor ShieldColor = FLinearColor(0.0f, 0.12f, 0.65f, 0.10f);

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Hero|Abilities", meta=(ClampMin="0"))
    float ShieldEmissiveIntensity = 2.0f;

    /** Shared recoil heat consumed per second while the shield is held. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Hero|Abilities", meta=(ClampMin="0"))
    float ShieldHeatPerSecond = 0.28f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Hero", meta=(ClampMin="0"))
    float WalkSpeed = 500.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Hero", meta=(ClampMin="0"))
    float CrouchedSpeed = 250.0f;

    /** Damage-dodge window that begins when the hero first enters crouch. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Hero|Combat", meta=(ClampMin="0"))
    float CrouchDamageImmunitySeconds = 2.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Hero", meta=(ClampMin="0"))
    float JumpVelocity = 600.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Hero", meta=(ClampMin="0"))
    float LookSensitivity = 1.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Hero", meta=(ClampMin="0.01"))
    float LookInputGraceSeconds = 0.12f;

    /** Mouse delta that fills a HUD axis meter end to end. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Hero", meta=(ClampMin="0.01"))
    float LookMeterRange = 8.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon", meta=(ClampMin="0"))
    float FireDamage = 25.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon", meta=(ClampMin="0"))
    float FireRange = 10000.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon", meta=(ClampMin="0.01"))
    float FireInterval = 0.2f;

    /**
     * Heat added by a single shot, as a fraction of the bar. At 0.125 the eighth shot in quick
     * succession overheats the weapon.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Recoil", meta=(ClampMin="0.001", ClampMax="1.0"))
    float RecoilHeatPerShot = 0.125f;

    /** Fraction of the bar that bleeds off per second while the trigger is not held. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Recoil", meta=(ClampMin="0.01"))
    float RecoilCooldownPerSecond = 0.4f;

    /** How long the weapon is locked out once the bar fills. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Recoil", meta=(ClampMin="0.1"))
    float RecoilOverheatSeconds = 3.0f;

    /** Bone or socket where the cosmetic tracer and muzzle flash begin. The hit trace remains camera-based. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon")
    FName VisualTraceOriginBoneName = TEXT("hand_r");

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Laser")
    TSoftObjectPtr<UMaterialInterface> LaserTraceMaterial;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Laser", meta=(ClampMin="0.1"))
    float LaserTraceThickness = 2.5f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Laser", meta=(ClampMin="0.01"))
    float LaserTraceLifetime = 0.12f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Laser")
    FLinearColor LaserTraceColor = FLinearColor(1.0f, 0.04f, 0.01f, 1.0f);

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Laser", meta=(ClampMin="0"))
    float LaserTraceIntensity = 35.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="UI|Crosshair", meta=(ClampMin="1"))
    float CrosshairDotSize = 6.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="UI|Crosshair")
    FLinearColor CrosshairDotColor = FLinearColor::White;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Zombie", meta=(ClampMin="0"))
    float ZombieMaxHealth = 50.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Zombie", meta=(ClampMin="0"))
    float ZombieMoveSpeed = 250.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Zombie", meta=(ClampMin="0"))
    float ZombieAttackRange = 140.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Zombie", meta=(ClampMin="0"))
    float ZombieAttackDamage = 10.0f;

    /** Radius of the melee line-of-sight sweep; world geometry and gates block damage. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Zombie|Combat", meta=(ClampMin="1"))
    float ZombieAttackTraceRadius = 24.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Zombie", meta=(ClampMin="0.01"))
    float ZombieAttackInterval = 1.0f;

    /** Delay between the attack tell/animation starting and damage being applied. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Zombie", meta=(ClampMin="0"))
    float ZombieAttackWindupSeconds = 0.65f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Zombie", meta=(ClampMin="0.01"))
    float ZombiePathRefreshInterval = 0.35f;

    /** Radius of the tactical ring zombies spread around instead of targeting one point. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Zombie|AI", meta=(ClampMin="50"))
    float ZombieEngagementRadius = 165.0f;

    /** Nearby-zombie distance used to push crowded agents apart. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Zombie|AI", meta=(ClampMin="50"))
    float ZombieSeparationRadius = 125.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Zombie|AI", meta=(ClampMin="0", ClampMax="3"))
    float ZombieSeparationStrength = 1.15f;

    /** How far ahead of a moving hero the tactical ring is positioned. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Zombie|AI", meta=(ClampMin="0", ClampMax="2"))
    float ZombieTargetPredictionSeconds = 0.32f;

    /** Repath immediately after the hero moves this far, even before the normal refresh. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Zombie|AI", meta=(ClampMin="5"))
    float ZombieReactiveRepathDistance = 45.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Zombie|AI", meta=(ClampMin="0.25"))
    float ZombieStuckRecoverySeconds = 1.0f;

    /** Dissolve-capable material used while a zombie assembles into the world. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Zombie|Spawn Effect")
    TSoftObjectPtr<UMaterialInterface> ZombieSpawnMaterial;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Zombie|Spawn Effect", meta=(ClampMin="0.05"))
    float ZombieSpawnEffectDuration = 0.85f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Zombie|Spawn Effect")
    FLinearColor ZombieSpawnEffectColor = FLinearColor(0.0f, 0.75f, 1.0f, 1.0f);

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Zombie|Spawn Effect", meta=(ClampMin="0"))
    float ZombieSpawnEffectIntensity = 5.0f;

    /** Controls how much of the assembly edge glows when supported by the assigned material. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Zombie|Spawn Effect", meta=(ClampMin="0.001", ClampMax="0.5"))
    float ZombieSpawnEdgeWidth = 0.08f;

    /** Brief red shader flash for a hit that does not kill the zombie. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Zombie|Hit Effect", meta=(ClampMin="0.01"))
    float ZombieHitFlashDuration = 0.11f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Zombie|Hit Effect")
    FLinearColor ZombieHitFlashColor = FLinearColor(1.0f, 0.015f, 0.0f, 1.0f);

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Zombie|Hit Effect", meta=(ClampMin="0"))
    float ZombieHitFlashIntensity = 3.0f;

    /** Masked unlit material used for the red blink and dissolve death effect. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Zombie|Death Effect")
    TSoftObjectPtr<UMaterialInterface> ZombieDeathMaterial;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Zombie|Death Effect", meta=(ClampMin="0"))
    float ZombieDeathBlinkDuration = 0.45f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Zombie|Death Effect", meta=(ClampMin="1"))
    float ZombieDeathBlinkFrequency = 18.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Zombie|Death Effect", meta=(ClampMin="0.05"))
    float ZombieDeathDissolveDuration = 0.75f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Network", meta=(ClampMin="1", ClampMax="2"))
    int32 RequiredPlayers = 2;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Network")
    FString SessionName = TEXT("Zombie Zero Two Player Co-op");

    /** Every font the built-in menu and HUD draw with. Set a font asset here to restyle both. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="UI|Fonts")
    FSlateFontInfo MenuTitleFont;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="UI|Fonts")
    FSlateFontInfo MenuButtonFont;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="UI|Fonts")
    FSlateFontInfo MenuStatusFont;

    /** Single-letter key caps (W, A, S, D) and the player number badges. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="UI|Fonts")
    FSlateFontInfo HudKeyFont;

    /** Multi-letter key caps (CTRL, SPACE, LMB). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="UI|Fonts")
    FSlateFontInfo HudKeyLabelFont;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="UI|Health")
    FLinearColor HeroHealthFillColor = FLinearColor(0.75f, 0.03f, 0.03f, 1.0f);

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="UI|Health")
    FLinearColor HeroHealthBackgroundColor = FLinearColor(0.025f, 0.025f, 0.025f, 0.9f);

    /** Recoil bar as it fills up. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="UI|Recoil")
    FLinearColor RecoilBarFillColor = FLinearColor(1.0f, 0.55f, 0.05f, 1.0f);

    /** Recoil bar once the weapon has overheated and is counting down. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="UI|Recoil")
    FLinearColor RecoilBarOverheatColor = FLinearColor(1.0f, 0.10f, 0.05f, 1.0f);

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="UI|Recoil")
    FLinearColor RecoilBarBackgroundColor = FLinearColor(0.025f, 0.025f, 0.025f, 0.9f);

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="UI|Damage Numbers")
    TSoftObjectPtr<UFont> DamageNumberFont;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="UI|Damage Numbers")
    FLinearColor DamageNumberColor = FLinearColor(1.0f, 0.12f, 0.04f, 1.0f);

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="UI|Damage Numbers", meta=(ClampMin="1"))
    float DamageNumberWorldSize = 30.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="UI|Damage Numbers", meta=(ClampMin="0"))
    float DamageNumberHeight = 125.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="UI|Damage Numbers", meta=(ClampMin="0"))
    float DamageNumberRiseSpeed = 45.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="UI|Damage Numbers", meta=(ClampMin="0.05"))
    float DamageNumberLifetime = 1.1f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="UI|Damage Feedback")
    FLinearColor HeroDamageVignetteColor = FLinearColor(0.8f, 0.0f, 0.0f, 0.58f);

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="UI|Damage Feedback", meta=(ClampMin="0.05"))
    float HeroDamageVignetteDuration = 0.45f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="UI|Damage Feedback")
    TSubclassOf<UCameraShakeBase> HeroDamageCameraShakeClass;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="UI|Damage Feedback", meta=(ClampMin="0"))
    float HeroDamageCameraShakeScale = 1.0f;

    /**
     * When true the shared hero is not spawned until every required player has connected, so
     * nobody starts playing alone. The host still opens the listen map as soon as the session
     * is created; only the start of play waits. Turn it off to begin the moment the map loads.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Network")
    bool bWaitForAllPlayersBeforeStart = true;
};
