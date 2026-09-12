#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineBaseTypes.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "EOSSessionSubsystem.generated.h"

class IOnlineSubsystem;
class UNetDriver;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FRazigraSessionStatus, const FString&, Status);

/** Hosts and joins a two-player advertised session through the configured online subsystem. */
UCLASS(BlueprintType)
class GAMEJAMRAZIGRA_API UEOSSessionSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    UFUNCTION(BlueprintCallable, Category="Razigra|Network")
    void HostGame();

    UFUNCTION(BlueprintCallable, Category="Razigra|Network")
    void FindAndJoinGame();

    /** Starts an offline test game where one player's input satisfies consensus. */
    UFUNCTION(BlueprintCallable, Category="Razigra|Network")
    void StartSinglePlayer();

    UFUNCTION(BlueprintCallable, Category="Razigra|Network")
    void CancelSearch();

    UFUNCTION(BlueprintPure, Category="Razigra|Network")
    FString GetOnlineSubsystemName() const;

    UFUNCTION(BlueprintPure, Category="Razigra|Network")
    bool ShouldShowMainMenu() const { return bMainMenuRequired; }

    UFUNCTION(BlueprintPure, Category="Razigra|Network")
    bool IsSinglePlayerMode() const { return bSinglePlayerMode; }

    /** True between opening the lobby and every required player being connected. */
    UFUNCTION(BlueprintPure, Category="Razigra|Network")
    bool IsWaitingForPlayers() const { return bWaitingForPlayer; }

    UFUNCTION(BlueprintPure, Category="Razigra|Network")
    int32 GetConnectedPlayers() const { return ConnectedPlayers; }

    UFUNCTION(BlueprintPure, Category="Razigra|Network")
    int32 GetExpectedPlayers() const { return ExpectedPlayers; }

    UFUNCTION(BlueprintPure, Category="Zombie Zero|Network")
    FString GetLastStatus() const { return LastStatus; }

    /** Called by the authoritative game mode as players enter or leave the session. */
    void NotifyPlayerCountChanged(int32 InConnectedPlayers, int32 InRequiredPlayers);

    /** Called by a client's game state when the replicated lobby population changes. */
    void ReportLobbyPopulation(int32 InConnectedPlayers, int32 InRequiredPlayers);

    /** Called once the shared hero exists on this machine, i.e. the match is really running. */
    void NotifyGameplayStarted();

    UPROPERTY(BlueprintAssignable, Category="Razigra|Network")
    FRazigraSessionStatus OnStatusChanged;

private:
    enum class EPendingOperation : uint8 { None, Host, Find };

    EPendingOperation PendingOperation = EPendingOperation::None;
    TSharedPtr<FOnlineSessionSearch> SessionSearch;
    FDelegateHandle LoginHandle;
    FDelegateHandle CreateHandle;
    FDelegateHandle DestroyHandle;
    FDelegateHandle FindHandle;
    FDelegateHandle JoinHandle;
    FDelegateHandle NetworkFailureHandle;
    FDelegateHandle TravelFailureHandle;
    FString LoginCredentialType;
    FString LoginId;
    FString LoginToken;
    FString LastStatus;
    bool bMainMenuRequired = true;
    bool bSinglePlayerMode = false;
    bool bWaitingForPlayer = false;
    int32 ConnectedPlayers = 0;
    int32 ExpectedPlayers = 2;

    void LoadExternalEOSConfig();
    IOnlineSubsystem* GetOnlineSubsystem() const;
    IOnlineSessionPtr GetSessions() const;
    void LoginThen(EPendingOperation Operation);
    void ContinuePendingOperation();
    void CreateSession();
    void BroadcastStatus(const FString& Status);
    void BeginGameplayTravel(bool bSinglePlayer, bool bHideMenu = true);
    void TravelToGameplayMap();
    FString GetGameplayMapName() const;
    void HandleNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType,
        const FString& Error);
    void HandleTravelFailure(UWorld* World, ETravelFailure::Type FailureType, const FString& Error);

    void HandleLoginComplete(int32 LocalUserNum, bool bWasSuccessful,
        const FUniqueNetId& UserId, const FString& Error);
    void HandleCreateSessionComplete(FName SessionName, bool bWasSuccessful);
    void HandleDestroySessionComplete(FName SessionName, bool bWasSuccessful);
    void HandleFindSessionsComplete(bool bWasSuccessful);
    void HandleJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
};
