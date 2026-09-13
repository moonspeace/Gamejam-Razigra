#include "GlobalGameData.h"

#include "CoopHudWidget.h"
#include "CoopMenuWidget.h"
#include "SharedHeroCharacter.h"
#include "ZombieCharacter.h"
#include "GamejamRazigra.h"
#include "HeroDamageCameraShake.h"
#include "Styling/CoreStyle.h"

namespace RazigraData
{
    static const TCHAR* AssetPath = TEXT("/Game/Data/GlobalGameData.GlobalGameData");
    static const TCHAR* DefaultHeroBlueprintPath = TEXT("/Game/Blueprints/BP_SharedHero.BP_SharedHero_C");
    static const TCHAR* ZombieDeathMaterialPath = TEXT("/Game/Materials/M_ZombieDeath.M_ZombieDeath");
    static const TCHAR* ZombieSpawnMaterialPath = TEXT("/Game/Materials/M_ZombieDeath.M_ZombieDeath");
    static const TCHAR* LaserTraceMaterialPath = TEXT("/Game/Materials/M_LaserTrace.M_LaserTrace");
    static const TCHAR* ShieldMaterialPath = TEXT("/Game/Materials/M_PlayerShield.M_PlayerShield");
    static const TCHAR* HealingMaterialPath = TEXT("/Game/Materials/M_HealingPlus.M_HealingPlus");
}

UGlobalGameData::UGlobalGameData()
    : MenuTitleFont(FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), 46))
    , MenuButtonFont(FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), 16))
    , MenuStatusFont(FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 12))
    , HudKeyFont(FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), 19))
    , HudKeyLabelFont(FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), 12))
{
    HeroBlueprint = TSoftClassPtr<ASharedHeroCharacter>(FSoftObjectPath(RazigraData::DefaultHeroBlueprintPath));
    ZombieClass = AZombieCharacter::StaticClass();
    MainMenuWidgetClass = UCoopMenuWidget::StaticClass();
    GameplayHudWidgetClass = UCoopHudWidget::StaticClass();
    GameplayMap = TSoftObjectPtr<UWorld>(FSoftObjectPath(TEXT("/Game/ThirdPerson/Lvl_ThirdPerson.Lvl_ThirdPerson")));
    ZombieMesh = TSoftObjectPtr<USkeletalMesh>(FSoftObjectPath(TEXT("/Game/ZombieMale_AAB/Meshes/SKM_ZombieM_Male_WholeBody.SKM_ZombieM_Male_WholeBody")));
    ZombieAnimationClass = TSoftClassPtr<UAnimInstance>(FSoftObjectPath(TEXT("/Game/ZombieMale_AAB/Animations/ABP_Zombie.ABP_Zombie_C")));
    DamageNumberFont = TSoftObjectPtr<UFont>(FSoftObjectPath(TEXT("/Engine/EngineFonts/Roboto.Roboto")));
    HeroDamageCameraShakeClass = UHeroDamageCameraShake::StaticClass();
    ZombieDeathMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(RazigraData::ZombieDeathMaterialPath));
    ZombieSpawnMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(RazigraData::ZombieSpawnMaterialPath));
    LaserTraceMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(RazigraData::LaserTraceMaterialPath));
    ShieldMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(RazigraData::ShieldMaterialPath));
    HealingEffectMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(RazigraData::HealingMaterialPath));
    ZombieAttackAnimation = TSoftObjectPtr<UAnimSequenceBase>(FSoftObjectPath(
        TEXT("/Game/ZombieMale_AAB/Animations/ZombieAttack_1__UE.ZombieAttack_1__UE")));
}

TSubclassOf<ASharedHeroCharacter> UGlobalGameData::GetHeroClass() const
{
    if (UClass* HeroClass = HeroBlueprint.LoadSynchronous())
    {
        return HeroClass;
    }
    UE_LOG(LogRazigra, Warning,
        TEXT("GlobalGameData.HeroBlueprint is not set (or failed to load); spawning the plain C++ hero, which has no mesh."));
    return ASharedHeroCharacter::StaticClass();
}

const UGlobalGameData* UGlobalGameData::Get(const UObject* WorldContextObject)
{
    static TWeakObjectPtr<const UGlobalGameData> CachedData;
    if (!CachedData.IsValid())
    {
        CachedData = LoadObject<UGlobalGameData>(nullptr, RazigraData::AssetPath);
        if (!CachedData.IsValid())
        {
            UE_LOG(LogRazigra, Error, TEXT("Missing required GlobalGameData asset at %s"), RazigraData::AssetPath);
            return GetDefault<UGlobalGameData>();
        }
    }
    return CachedData.Get();
}
