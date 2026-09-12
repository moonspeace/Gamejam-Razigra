#include "GlobalGameData.h"

#include "CoopHudWidget.h"
#include "CoopMenuWidget.h"
#include "SharedHeroCharacter.h"
#include "ZombieCharacter.h"
#include "GamejamRazigra.h"

namespace RazigraData
{
    static const TCHAR* AssetPath = TEXT("/Game/Data/GlobalGameData.GlobalGameData");
    static const TCHAR* DefaultHeroBlueprintPath = TEXT("/Game/Blueprints/BP_SharedHero.BP_SharedHero_C");
}

UGlobalGameData::UGlobalGameData()
{
    HeroBlueprint = TSoftClassPtr<ASharedHeroCharacter>(FSoftObjectPath(RazigraData::DefaultHeroBlueprintPath));
    ZombieClass = AZombieCharacter::StaticClass();
    MainMenuWidgetClass = UCoopMenuWidget::StaticClass();
    GameplayHudWidgetClass = UCoopHudWidget::StaticClass();
    GameplayMap = TSoftObjectPtr<UWorld>(FSoftObjectPath(TEXT("/Game/ThirdPerson/Lvl_ThirdPerson.Lvl_ThirdPerson")));
    ZombieMesh = TSoftObjectPtr<USkeletalMesh>(FSoftObjectPath(TEXT("/Game/ZombieMale_AAB/Meshes/SKM_ZombieM_Male_WholeBody.SKM_ZombieM_Male_WholeBody")));
    ZombieAnimationClass = TSoftClassPtr<UAnimInstance>(FSoftObjectPath(TEXT("/Game/ZombieMale_AAB/Animations/ABP_Zombie.ABP_Zombie_C")));
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
