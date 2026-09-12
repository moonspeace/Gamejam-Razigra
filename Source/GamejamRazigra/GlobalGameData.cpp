#include "GlobalGameData.h"

#include "CoopMenuWidget.h"
#include "SharedHeroCharacter.h"
#include "ZombieCharacter.h"
#include "GamejamRazigra.h"

namespace RazigraData
{
    static const TCHAR* AssetPath = TEXT("/Game/Data/GlobalGameData.GlobalGameData");
}

UGlobalGameData::UGlobalGameData()
{
    HeroClass = ASharedHeroCharacter::StaticClass();
    ZombieClass = AZombieCharacter::StaticClass();
    MainMenuWidgetClass = UCoopMenuWidget::StaticClass();
    GameplayMap = TSoftObjectPtr<UWorld>(FSoftObjectPath(TEXT("/Game/ThirdPerson/Lvl_ThirdPerson.Lvl_ThirdPerson")));
    HeroMesh = TSoftObjectPtr<USkeletalMesh>(FSoftObjectPath(TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple")));
    HeroAnimationClass = TSoftClassPtr<UAnimInstance>(FSoftObjectPath(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed.ABP_Unarmed_C")));
    ZombieMesh = TSoftObjectPtr<USkeletalMesh>(FSoftObjectPath(TEXT("/Game/ZombieMale_AAB/Meshes/SKM_ZombieM_Male_WholeBody.SKM_ZombieM_Male_WholeBody")));
    ZombieAnimationClass = TSoftClassPtr<UAnimInstance>(FSoftObjectPath(TEXT("/Game/ZombieMale_AAB/Animations/ABP_Zombie.ABP_Zombie_C")));
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
