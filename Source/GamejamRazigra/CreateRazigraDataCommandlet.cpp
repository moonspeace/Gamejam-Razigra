#include "CreateRazigraDataCommandlet.h"

#include "GamejamRazigra.h"
#include "GlobalGameData.h"

#if WITH_EDITOR
#include "AssetRegistry/AssetRegistryModule.h"
#include "CoopGameMode.h"
#include "FileHelpers.h"
#include "GameFramework/WorldSettings.h"
#include "Misc/PackageName.h"
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

int32 UCreateRazigraDataCommandlet::Main(const FString& Params)
{
#if WITH_EDITOR
    const FString PackageName(TEXT("/Game/Data/GlobalGameData"));
    const FString AssetName(TEXT("GlobalGameData"));
    const FString Filename = FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension());

    bool bSaved = true;
    UGlobalGameData* DataAsset = nullptr;
    if (!FPaths::FileExists(Filename))
    {
        UPackage* Package = CreatePackage(*PackageName);
        DataAsset = NewObject<UGlobalGameData>(Package, *AssetName, RF_Public | RF_Standalone);
        FAssetRegistryModule::AssetCreated(DataAsset);
        Package->MarkPackageDirty();

        FSavePackageArgs SaveArgs;
        SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
        SaveArgs.SaveFlags = SAVE_NoError;
        bSaved = UPackage::SavePackage(Package, DataAsset, *Filename, SaveArgs);
        UE_LOG(LogRazigra, Display, TEXT("Created GlobalGameData: %s"), *Filename);
    }
    else
    {
        DataAsset = LoadObject<UGlobalGameData>(nullptr, TEXT("/Game/Data/GlobalGameData.GlobalGameData"));
        UE_LOG(LogRazigra, Display, TEXT("GlobalGameData already exists: %s"), *Filename);
        if (DataAsset)
        {
            DataAsset->MarkPackageDirty();
            FSavePackageArgs SaveArgs;
            SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
            SaveArgs.SaveFlags = SAVE_NoError;
            bSaved &= UPackage::SavePackage(DataAsset->GetPackage(), DataAsset, *Filename, SaveArgs);
        }
    }

    const FString GameplayMap(TEXT("/Game/ThirdPerson/Lvl_ThirdPerson"));
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
