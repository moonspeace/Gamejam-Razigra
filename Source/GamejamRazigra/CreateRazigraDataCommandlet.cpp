#include "CreateRazigraDataCommandlet.h"

#include "GamejamRazigra.h"
#include "GlobalGameData.h"

#if WITH_EDITOR
#include "Animation/AnimInstance.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Components/SkeletalMeshComponent.h"
#include "CoopGameMode.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/SkeletalMesh.h"
#include "FileHelpers.h"
#include "GameFramework/WorldSettings.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "SharedHeroCharacter.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#endif

UCreateRazigraDataCommandlet::UCreateRazigraDataCommandlet()
{
    IsClient = false;
    IsServer = false;
    IsEditor = true;
    LogToConsole = true;
}

#if WITH_EDITOR
namespace RazigraCommandlet
{
    static const TCHAR* HeroBlueprintPackage = TEXT("/Game/Blueprints/BP_SharedHero");
    static const TCHAR* HeroBlueprintObject = TEXT("/Game/Blueprints/BP_SharedHero.BP_SharedHero");
    static const TCHAR* HeroMeshPath = TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple");
    static const TCHAR* HeroAnimationPath = TEXT("/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed.ABP_Unarmed_C");

    static bool SavePackageTo(UPackage* Package, UObject* Asset, const FString& Filename)
    {
        Package->MarkPackageDirty();
        FSavePackageArgs SaveArgs;
        SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
        SaveArgs.SaveFlags = SAVE_NoError;
        return UPackage::SavePackage(Package, Asset, *Filename, SaveArgs);
    }

    /**
     * The hero's visuals live on a Blueprint rather than in GlobalGameData, so create a
     * starter Blueprint with the sample mannequin already assigned if one is not present.
     */
    static UBlueprint* EnsureHeroBlueprint()
    {
        const FString Filename = FPackageName::LongPackageNameToFilename(HeroBlueprintPackage,
            FPackageName::GetAssetPackageExtension());
        if (FPaths::FileExists(Filename))
        {
            UE_LOG(LogRazigra, Display, TEXT("Hero Blueprint already exists: %s"), *Filename);
            return LoadObject<UBlueprint>(nullptr, HeroBlueprintObject);
        }

        UPackage* Package = CreatePackage(HeroBlueprintPackage);
        UBlueprint* Blueprint = FKismetEditorUtilities::CreateBlueprint(
            ASharedHeroCharacter::StaticClass(), Package, TEXT("BP_SharedHero"), BPTYPE_Normal,
            UBlueprint::StaticClass(), UBlueprintGeneratedClass::StaticClass());
        if (!Blueprint)
        {
            UE_LOG(LogRazigra, Error, TEXT("Could not create the hero Blueprint at %s"), HeroBlueprintPackage);
            return nullptr;
        }
        FAssetRegistryModule::AssetCreated(Blueprint);

        if (ASharedHeroCharacter* HeroDefaults = Blueprint->GeneratedClass
            ? Cast<ASharedHeroCharacter>(Blueprint->GeneratedClass->GetDefaultObject()) : nullptr)
        {
            if (USkeletalMeshComponent* MeshComponent = HeroDefaults->GetMesh())
            {
                if (USkeletalMesh* MeshAsset = LoadObject<USkeletalMesh>(nullptr, HeroMeshPath))
                {
                    MeshComponent->SetSkeletalMeshAsset(MeshAsset);
                }
                else
                {
                    UE_LOG(LogRazigra, Warning, TEXT("Sample hero mesh %s was not found; assign one on %s."),
                        HeroMeshPath, HeroBlueprintObject);
                }
                if (UClass* AnimationClass = LoadClass<UAnimInstance>(nullptr, HeroAnimationPath))
                {
                    MeshComponent->SetAnimInstanceClass(AnimationClass);
                }
            }
        }

        FKismetEditorUtilities::CompileBlueprint(Blueprint);
        if (!SavePackageTo(Package, Blueprint, Filename))
        {
            UE_LOG(LogRazigra, Error, TEXT("Could not save the hero Blueprint to %s"), *Filename);
            return Blueprint;
        }
        UE_LOG(LogRazigra, Display, TEXT("Created hero Blueprint: %s"), *Filename);
        return Blueprint;
    }
}
#endif

int32 UCreateRazigraDataCommandlet::Main(const FString& Params)
{
#if WITH_EDITOR
    const FString PackageName(TEXT("/Game/Data/GlobalGameData"));
    const FString AssetName(TEXT("GlobalGameData"));
    const FString Filename = FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension());

    UBlueprint* HeroBlueprint = RazigraCommandlet::EnsureHeroBlueprint();

    bool bSaved = true;
    UGlobalGameData* DataAsset = nullptr;
    if (!FPaths::FileExists(Filename))
    {
        UPackage* Package = CreatePackage(*PackageName);
        DataAsset = NewObject<UGlobalGameData>(Package, *AssetName, RF_Public | RF_Standalone);
        FAssetRegistryModule::AssetCreated(DataAsset);
        UE_LOG(LogRazigra, Display, TEXT("Created GlobalGameData: %s"), *Filename);
    }
    else
    {
        DataAsset = LoadObject<UGlobalGameData>(nullptr, TEXT("/Game/Data/GlobalGameData.GlobalGameData"));
        UE_LOG(LogRazigra, Display, TEXT("GlobalGameData already exists: %s"), *Filename);
    }

    if (DataAsset)
    {
        if (HeroBlueprint && HeroBlueprint->GeneratedClass)
        {
            const TSoftClassPtr<ASharedHeroCharacter> HeroClass(
                FSoftObjectPath(HeroBlueprint->GeneratedClass->GetPathName()));
            if (DataAsset->HeroBlueprint != HeroClass)
            {
                DataAsset->HeroBlueprint = HeroClass;
                UE_LOG(LogRazigra, Display, TEXT("Pointed GlobalGameData.HeroBlueprint at %s"),
                    *HeroBlueprint->GeneratedClass->GetPathName());
            }
        }
        bSaved &= RazigraCommandlet::SavePackageTo(DataAsset->GetPackage(), DataAsset, Filename);
    }
    else
    {
        bSaved = false;
    }

    FString GameplayMap(TEXT("/Game/ThirdPerson/Lvl_ThirdPerson"));
    if (DataAsset && !DataAsset->GameplayMap.IsNull())
    {
        GameplayMap = DataAsset->GameplayMap.ToSoftObjectPath().GetLongPackageName();
    }
    if (UWorld* World = UEditorLoadingAndSavingUtils::LoadMap(GameplayMap))
    {
        if (World->GetWorldSettings()->DefaultGameMode != ACoopGameMode::StaticClass())
        {
            World->GetWorldSettings()->DefaultGameMode = ACoopGameMode::StaticClass();
            World->GetWorldSettings()->MarkPackageDirty();
            bSaved &= UEditorLoadingAndSavingUtils::SaveMap(World, GameplayMap);
        }
    }
    else
    {
        UE_LOG(LogRazigra, Error, TEXT("Could not load gameplay map %s"), *GameplayMap);
        bSaved = false;
    }
    return bSaved ? 0 : 1;
#else
    return 1;
#endif
}
