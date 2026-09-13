#include "CoopGameState.h"

#include "CoopPlayerController.h"
#include "EOSSessionSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Net/UnrealNetwork.h"
#include "GlobalGameData.h"

void ACoopGameState::AddZombieKill()
{
    if (HasAuthority())
    {
        ++ZombieKillCount;
        ForceNetUpdate();
    }
}

void ACoopGameState::ResetZombieKills()
{
    if (HasAuthority())
    {
        ZombieKillCount = 0;
        ForceNetUpdate();
    }
}

void ACoopGameState::StartIntroCinematic()
{
    if (!HasAuthority() || CinematicStartServerTime >= 0.0
        || UGlobalGameData::Get(this)->LevelIntroCinematic.IsNull())
    {
        return;
    }
    CinematicStartServerTime = GetServerWorldTimeSeconds()
        + UGlobalGameData::Get(this)->IntroCinematicStartDelay;
    OnRep_CinematicStartTime();
    ForceNetUpdate();
}

void ACoopGameState::OnRep_CinematicStartTime()
{
    if (CinematicStartServerTime < 0.0 || !GetWorld())
    {
        return;
    }
    for (TActorIterator<ACoopPlayerController> It(GetWorld()); It; ++It)
    {
        if (It->IsLocalController())
        {
            It->StartLevelCinematic(CinematicStartServerTime);
        }
    }
}

void ACoopGameState::OnRep_SharedHero()
{
    if (!SharedHero)
    {
        return;
    }
    for (TActorIterator<ACoopPlayerController> It(GetWorld()); It; ++It)
    {
        if (It->IsLocalController())
        {
            It->BindToSharedHero(SharedHero);
        }
    }
}

void ACoopGameState::OnRep_LobbyPopulation()
{
    if (UGameInstance* Instance = GetGameInstance())
    {
        if (UEOSSessionSubsystem* Sessions = Instance->GetSubsystem<UEOSSessionSubsystem>())
        {
            Sessions->ReportLobbyPopulation(ConnectedPlayerCount, RequiredPlayerCount);
        }
    }
}

void ACoopGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ACoopGameState, SharedHero);
    DOREPLIFETIME(ACoopGameState, ConnectedPlayerCount);
    DOREPLIFETIME(ACoopGameState, RequiredPlayerCount);
    DOREPLIFETIME(ACoopGameState, ZombieKillCount);
    DOREPLIFETIME(ACoopGameState, CinematicStartServerTime);
}
