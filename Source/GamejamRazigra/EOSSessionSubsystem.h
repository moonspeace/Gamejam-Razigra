#pragma once

#include "CoreMinimal.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "EOSSessionSubsystem.generated.h"

class IOnlineSubsystem;

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
    FString LoginCredentialType;
    FString LoginId;
    FString LoginToken;
    bool bMainMenuRequired = true;
    bool bSinglePlayerMode = false;

    void LoadExternalEOSConfig();
    IOnlineSubsystem* GetOnlineSubsystem() const;
    IOnlineSessionPtr GetSessions() const;
    void LoginThen(EPendingOperation Operation);
    void ContinuePendingOperation();
    void CreateSession();
    void BroadcastStatus(const FString& Status);
    void BeginGameplayTravel(bool bSinglePlayer);

    void HandleLoginComplete(int32 LocalUserNum, bool bWasSuccessful,
        const FUniqueNetId& UserId, const FString& Error);
    void HandleCreateSessionComplete(FName SessionName, bool bWasSuccessful);
    void HandleDestroySessionComplete(FName SessionName, bool bWasSuccessful);
    void HandleFindSessionsComplete(bool bWasSuccessful);
    void HandleJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
};
