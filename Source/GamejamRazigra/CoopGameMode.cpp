#include "CoopGameMode.h"

#include "CoopGameState.h"
#include "CoopPlayerController.h"
#include "EOSSessionSubsystem.h"
#include "Engine/GameInstance.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerStart.h"
#include "GlobalGameData.h"
#include "GamejamRazigra.h"
#include "SharedHeroCharacter.h"

ACoopGameMode::ACoopGameMode()
{
    DefaultPawnClass = nullptr;
    PlayerControllerClass = ACoopPlayerController::StaticClass();
    GameStateClass = ACoopGameState::StaticClass();
    bUseSeamlessTravel = true;
}

void ACoopGameMode::StartPlay()
{
    Super::StartPlay();
    if (CanStartGameplay())
    {
        EnsureSharedHero();
    }
}

int32 ACoopGameMode::GetRequiredPlayers() const
{
    const UGameInstance* Instance = GetGameInstance();
    const UEOSSessionSubsystem* Sessions = Instance ? Instance->GetSubsystem<UEOSSessionSubsystem>() : nullptr;
    if (Sessions && Sessions->IsSinglePlayerMode())
    {
        return 1;
    }
    return FMath::Max(1, UGlobalGameData::Get(this)->RequiredPlayers);
}

/**
 * The listen map is opened as soon as the session exists, so being here does not mean the game
 * has started. The hero is held back until every player has actually connected; that wait is
 * purely server-side bookkeeping and never touches travel or the net connection.
 */
bool ACoopGameMode::CanStartGameplay() const
{
    if (!GetWorld())
    {
        return false;
    }
    // AGameModeBase::GetNumPlayers() is not const, and AssignedSlots is the same population
    // counted the same way on every net mode, so use it for the gate everywhere.
    const bool bEveryoneConnected = AssignedSlots.Num() >= GetRequiredPlayers();
    if (UGlobalGameData::Get(this)->bWaitForAllPlayersBeforeStart && !bEveryoneConnected)
    {
        return false;
    }
    if (GetWorld()->GetNetMode() == NM_DedicatedServer)
    {
        return true;
    }
    const UGameInstance* Instance = GetGameInstance();
    const UEOSSessionSubsystem* Sessions = Instance ? Instance->GetSubsystem<UEOSSessionSubsystem>() : nullptr;
    return Sessions && !Sessions->ShouldShowMainMenu();
}

void ACoopGameMode::EnsureSharedHero()
{
    ACoopGameState* State = GetGameState<ACoopGameState>();
    if (!State || State->SharedHero)
    {
        return;
    }

    FVector SpawnLocation(0.0f, 0.0f, 150.0f);
    FRotator SpawnRotation = FRotator::ZeroRotator;
    for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
    {
        SpawnLocation = It->GetActorLocation();
        SpawnRotation = It->GetActorRotation();
        break;
    }

    const UGlobalGameData* Data = UGlobalGameData::Get(this);
    UE_LOG(LogRazigra, Log, TEXT("Spawning shared hero at %s."), *SpawnLocation.ToCompactString());
    State->SharedHero = GetWorld()->SpawnActor<ASharedHeroCharacter>(Data->GetHeroClass(), SpawnLocation, SpawnRotation);
    if (State->SharedHero)
    {
        State->SharedHero->SetRequiredConsensusParticipants(GetRequiredPlayers());
        BP_OnSharedHeroSpawned(State->SharedHero);
        for (const TPair<TWeakObjectPtr<ACoopPlayerController>, int32>& Pair : AssignedSlots)
        {
            if (ACoopPlayerController* Controller = Pair.Key.Get())
            {
                Controller->ClientBindToSharedHero(State->SharedHero);
            }
        }
    }
}

void ACoopGameMode::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);
    ACoopPlayerController* CoopController = Cast<ACoopPlayerController>(NewPlayer);
    if (!CoopController)
    {
        return;
    }

    RegisterPlayer(CoopController);

    if (CanStartGameplay())
    {
        EnsureSharedHero();
        if (const ACoopGameState* State = GetGameState<ACoopGameState>())
        {
            CoopController->ClientBindToSharedHero(State->SharedHero);
        }
    }
}

void ACoopGameMode::HandleSeamlessTravelPlayer(AController*& Controller)
{
    Super::HandleSeamlessTravelPlayer(Controller);
    if (ACoopPlayerController* CoopController = Cast<ACoopPlayerController>(Controller))
    {
        RegisterPlayer(CoopController);
        if (CanStartGameplay())
        {
            EnsureSharedHero();
        }
    }
}

void ACoopGameMode::RegisterPlayer(ACoopPlayerController* CoopController)
{
    if (!CoopController || AssignedSlots.Contains(CoopController))
    {
        return;
    }
    const int32 Slot = AllocateSlot();
    if (Slot == INDEX_NONE)
    {
        return;
    }
    AssignedSlots.Add(CoopController, Slot);
    CoopController->SetPlayerSlot(Slot);
    UE_LOG(LogRazigra, Log, TEXT("Player %s assigned consensus slot %d."), *CoopController->GetName(), Slot);
    RefreshConnectedCount();
}

void ACoopGameMode::Logout(AController* Exiting)
{
    if (ACoopPlayerController* CoopController = Cast<ACoopPlayerController>(Exiting))
    {
        if (const int32* Slot = AssignedSlots.Find(CoopController))
        {
            if (const ACoopGameState* State = GetGameState<ACoopGameState>())
            {
                if (State->SharedHero)
                {
                    State->SharedHero->ResetParticipant(*Slot);
                }
            }
        }
        AssignedSlots.Remove(CoopController);
        UE_LOG(LogRazigra, Log, TEXT("Player %s left the consensus game."), *CoopController->GetName());
    }
    Super::Logout(Exiting);
    RefreshConnectedCount();
}

void ACoopGameMode::PreLogin(const FString& Options, const FString& Address,
    const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
    Super::PreLogin(Options, Address, UniqueId, ErrorMessage);
    if (ErrorMessage.IsEmpty() && GetNumPlayers() >= GetRequiredPlayers())
    {
        ErrorMessage = TEXT("Server is full (this game requires exactly two players).");
    }
}

int32 ACoopGameMode::AllocateSlot() const
{
    for (int32 Candidate = 0; Candidate < 2; ++Candidate)
    {
        bool bUsed = false;
        for (const TPair<TWeakObjectPtr<ACoopPlayerController>, int32>& Pair : AssignedSlots)
        {
            bUsed |= Pair.Value == Candidate;
        }
        if (!bUsed)
        {
            return Candidate;
        }
    }
    return INDEX_NONE;
}

void ACoopGameMode::RefreshConnectedCount()
{
    const int32 Required = GetRequiredPlayers();
    if (ACoopGameState* State = GetGameState<ACoopGameState>())
    {
        State->ConnectedPlayerCount = AssignedSlots.Num();
        State->RequiredPlayerCount = Required;
    }
    if (UGameInstance* Instance = GetGameInstance())
    {
        if (UEOSSessionSubsystem* Sessions = Instance->GetSubsystem<UEOSSessionSubsystem>())
        {
            Sessions->NotifyPlayerCountChanged(AssignedSlots.Num(), Required);
        }
    }
}
