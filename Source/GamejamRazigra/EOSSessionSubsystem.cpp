#include "EOSSessionSubsystem.h"

#include "Engine/GameInstance.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "EOSSettings.h"
#include "GameFramework/PlayerController.h"
#include "GamejamRazigra.h"
#include "GlobalGameData.h"
#include "HAL/IConsoleManager.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Online/OnlineSessionNames.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "OnlineSessionSettings.h"
#include "RazigraGameInstance.h"
#include "Kismet/GameplayStatics.h"

namespace RazigraSessions
{
    const FName SessionName = NAME_GameSession;
    const FName GameKey(TEXT("RAZIGRA_GAME"));
}

void UEOSSessionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    LoadExternalEOSConfig();

#if WITH_EDITOR
    // PIE disables seamless travel by default. Without it, the lobby's travel to the gameplay
    // map is a hard travel that drops every connected client, so force it on in the editor.
    if (IConsoleVariable* AllowPIESeamlessTravel =
        IConsoleManager::Get().FindConsoleVariable(TEXT("net.AllowPIESeamlessTravel")))
    {
        if (AllowPIESeamlessTravel->GetInt() == 0)
        {
            AllowPIESeamlessTravel->Set(TEXT("1"), ECVF_SetByCode);
            UE_LOG(LogRazigra, Log,
                TEXT("Enabled net.AllowPIESeamlessTravel so PIE clients survive the travel out of the lobby."));
        }
    }
#endif

    if (GEngine)
    {
        NetworkFailureHandle = GEngine->OnNetworkFailure().AddUObject(this, &ThisClass::HandleNetworkFailure);
        TravelFailureHandle = GEngine->OnTravelFailure().AddUObject(this, &ThisClass::HandleTravelFailure);
    }

    const IOnlineSubsystem* Online = GetOnlineSubsystem();
    const bool bHasIdentity = Online && Online->GetIdentityInterface().IsValid();
    const bool bHasSessions = Online && Online->GetSessionInterface().IsValid();
    BroadcastStatus(FString::Printf(TEXT("Online service: %s | Identity: %s | Sessions: %s"),
        *GetOnlineSubsystemName(), bHasIdentity ? TEXT("ready") : TEXT("missing"),
        bHasSessions ? TEXT("ready") : TEXT("missing")));
}

void UEOSSessionSubsystem::Deinitialize()
{
    if (GEngine)
    {
        GEngine->OnNetworkFailure().Remove(NetworkFailureHandle);
        GEngine->OnTravelFailure().Remove(TravelFailureHandle);
    }
    if (IOnlineSubsystem* Online = GetOnlineSubsystem())
    {
        if (IOnlineIdentityPtr Identity = Online->GetIdentityInterface())
        {
            Identity->ClearOnLoginCompleteDelegate_Handle(0, LoginHandle);
        }
    }
    CancelSearch();
    Super::Deinitialize();
}

void UEOSSessionSubsystem::LoadExternalEOSConfig()
{
    const FString ConfigPath = FPaths::Combine(FPaths::ProjectConfigDir(), TEXT("EOS.ini"));
    if (!FPaths::FileExists(ConfigPath))
    {
        UE_LOG(LogRazigra, Warning, TEXT("%s does not exist; OnlineSubsystemNull will be used for local testing."), *ConfigPath);
        return;
    }

    FConfigFile ExternalConfig;
    ExternalConfig.Read(ConfigPath);
    for (const TPair<FString, FConfigSection>& SectionPair : ExternalConfig)
    {
        TSet<FName> ReplacedKeys;
        for (const TPair<FName, FConfigValue>& ValuePair : SectionPair.Value)
        {
            if (!ReplacedKeys.Contains(ValuePair.Key))
            {
                GConfig->RemoveKeyFromSection(*SectionPair.Key, ValuePair.Key, GEngineIni);
                ReplacedKeys.Add(ValuePair.Key);
            }
            GConfig->AddToSection(*SectionPair.Key, ValuePair.Key, ValuePair.Value.GetValue(), GEngineIni);
        }
    }

    GConfig->GetString(TEXT("GamejamRazigra.EOS"), TEXT("LoginCredentialType"), LoginCredentialType, GEngineIni);
    GConfig->GetString(TEXT("GamejamRazigra.EOS"), TEXT("LoginId"), LoginId, GEngineIni);
    GConfig->GetString(TEXT("GamejamRazigra.EOS"), TEXT("LoginToken"), LoginToken, GEngineIni);
    GetMutableDefault<UEOSSettings>()->ReloadConfig();
    const UEOSSettings* EOSSettings = GetDefault<UEOSSettings>();
    UE_LOG(LogRazigra, Log, TEXT("EOS settings refreshed: default artifact '%s', %d artifact(s), credential mode '%s'."),
        *EOSSettings->DefaultArtifactName, EOSSettings->Artifacts.Num(),
        LoginCredentialType.IsEmpty() ? TEXT("automatic") : *LoginCredentialType);
    // OnlineSubsystem selects its default before GameInstance subsystems initialize.
    // Rebuild that default now that the deliberately external credentials are in memory.
    IOnlineSubsystem::ReloadDefaultSubsystem();
    if (GEngine && IOnlineSubsystem::Get() && IOnlineSubsystem::Get()->GetSubsystemName() == TEXT("EOS"))
    {
        for (FNetDriverDefinition& Driver : GEngine->NetDriverDefinitions)
        {
            if (Driver.DefName == NAME_GameNetDriver)
            {
                Driver.DriverClassName = FName(TEXT("/Script/OnlineSubsystemEOS.NetDriverEOS"));
                Driver.DriverClassNameFallback = FName(TEXT("/Script/OnlineSubsystemUtils.IpNetDriver"));
            }
        }
    }
    UE_LOG(LogRazigra, Log, TEXT("Loaded external EOS configuration from %s"), *ConfigPath);
}

IOnlineSubsystem* UEOSSessionSubsystem::GetOnlineSubsystem() const
{
    // In editor/PIE, EOS creates one subsystem per world context. The EOS net driver
    // resolves its socket subsystem the same way, so authentication and sessions must
    // use this instance rather than the global default or P2P bind has no local user.
    return Online::GetSubsystem(GetWorld());
}

IOnlineSessionPtr UEOSSessionSubsystem::GetSessions() const
{
    if (IOnlineSubsystem* Online = GetOnlineSubsystem())
    {
        return Online->GetSessionInterface();
    }
    return nullptr;
}

FString UEOSSessionSubsystem::GetOnlineSubsystemName() const
{
    if (const IOnlineSubsystem* Online = GetOnlineSubsystem())
    {
        return Online->GetSubsystemName().ToString();
    }
    return TEXT("Unavailable");
}

void UEOSSessionSubsystem::BroadcastStatus(const FString& Status)
{
    LastStatus = Status;
    UE_LOG(LogRazigra, Log, TEXT("Session: %s"), *Status);
    // Status is surfaced through the menu/HUD widgets only; nothing is drawn over the game.
    OnStatusChanged.Broadcast(Status);
}

void UEOSSessionSubsystem::HostGame()
{
    LoginThen(EPendingOperation::Host);
}

void UEOSSessionSubsystem::FindAndJoinGame()
{
    LoginThen(EPendingOperation::Find);
}

void UEOSSessionSubsystem::StartSinglePlayer()
{
    BroadcastStatus(TEXT("Starting single-player test game..."));
    bWaitingForPlayer = false;
    ConnectedPlayers = 1;
    ExpectedPlayers = 1;
    BeginGameplayTravel(true);
    UGameplayStatics::OpenLevel(this, FName(*GetGameplayMapName()), true);
}

FString UEOSSessionSubsystem::GetGameplayMapName() const
{
    return UGlobalGameData::Get(this)->GameplayMap.ToSoftObjectPath().GetLongPackageName();
}

bool UEOSSessionSubsystem::IsOnGameplayMap() const
{
    const UWorld* World = GetWorld();
    if (!World)
    {
        return false;
    }
    const FString Current = UWorld::RemovePIEPrefix(World->GetMapName());
    const FString Gameplay = FPackageName::GetShortName(GetGameplayMapName());
    return !Gameplay.IsEmpty() && Current.Equals(Gameplay, ESearchCase::IgnoreCase);
}

/**
 * Opens the current map for connections without travelling anywhere. The host stays in the
 * front end, advertising the session, until every player has joined.
 */
bool UEOSSessionSubsystem::StartListening()
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return false;
    }
    if (World->GetNetMode() == NM_ListenServer || World->GetNetMode() == NM_DedicatedServer)
    {
        return true;
    }

    FURL ListenURL(nullptr, TEXT(""), TRAVEL_Absolute);
    ListenURL.AddOption(TEXT("listen"));
    if (!World->Listen(ListenURL))
    {
        return false;
    }
    World->URL.AddOption(TEXT("listen"));
    UE_LOG(LogRazigra, Log, TEXT("Lobby is listening on map %s; waiting for players before travelling."),
        *World->GetMapName());
    return true;
}

void UEOSSessionSubsystem::TravelToGameplayMap()
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }
    const FString Map = GetGameplayMapName();
    UE_LOG(LogRazigra, Log, TEXT("All players connected; travelling to %s."), *Map);
    World->ServerTravel(Map + TEXT("?listen"));
}

void UEOSSessionSubsystem::BeginGameplayTravel(bool bSinglePlayer, bool bHideMenu)
{
    bMainMenuRequired = false;
    bSinglePlayerMode = bSinglePlayer;
    if (bHideMenu)
    {
        if (URazigraGameInstance* RazigraInstance = Cast<URazigraGameInstance>(GetGameInstance()))
        {
            RazigraInstance->HideMainMenu();
        }
    }
}

void UEOSSessionSubsystem::LoginThen(EPendingOperation Operation)
{
    IOnlineSubsystem* Online = GetOnlineSubsystem();
    if (!Online)
    {
        BroadcastStatus(TEXT("No online subsystem is available."));
        return;
    }

    PendingOperation = Operation;
    UE_LOG(LogRazigra, Log, TEXT("Starting %s flow using subsystem %s; login status=%d."),
        Operation == EPendingOperation::Host ? TEXT("host") : TEXT("join"),
        *Online->GetSubsystemName().ToString(), static_cast<int32>(Online->GetIdentityInterface().IsValid()
            ? Online->GetIdentityInterface()->GetLoginStatus(0) : ELoginStatus::NotLoggedIn));
    IOnlineIdentityPtr Identity = Online->GetIdentityInterface();
    if (!Identity.IsValid() || Online->GetSubsystemName() == TEXT("NULL") ||
        Identity->GetLoginStatus(0) == ELoginStatus::LoggedIn)
    {
        ContinuePendingOperation();
        return;
    }

    Identity->ClearOnLoginCompleteDelegate_Handle(0, LoginHandle);
    LoginHandle = Identity->AddOnLoginCompleteDelegate_Handle(0,
        FOnLoginCompleteDelegate::CreateUObject(this, &ThisClass::HandleLoginComplete));

    BroadcastStatus(TEXT("Signing in to Epic Online Services..."));
    bool bStarted = false;
    if (!LoginCredentialType.IsEmpty())
    {
        bStarted = Identity->Login(0, FOnlineAccountCredentials(LoginCredentialType, LoginId, LoginToken));
    }
    else
    {
        bStarted = Identity->AutoLogin(0);
    }
    if (!bStarted)
    {
        BroadcastStatus(TEXT("EOS login could not start. Check Config/EOS.ini and launch credentials."));
        PendingOperation = EPendingOperation::None;
    }
}

void UEOSSessionSubsystem::HandleLoginComplete(int32 LocalUserNum, bool bWasSuccessful,
    const FUniqueNetId& UserId, const FString& Error)
{
    if (IOnlineSubsystem* Online = GetOnlineSubsystem())
    {
        if (IOnlineIdentityPtr Identity = Online->GetIdentityInterface())
        {
            Identity->ClearOnLoginCompleteDelegate_Handle(LocalUserNum, LoginHandle);
        }
    }
    if (!bWasSuccessful)
    {
        BroadcastStatus(FString::Printf(TEXT("EOS sign-in failed: %s"), *Error));
        PendingOperation = EPendingOperation::None;
        return;
    }
    UE_LOG(LogRazigra, Log, TEXT("EOS login succeeded for local user %d (account %s)."),
        LocalUserNum, *UserId.ToDebugString());
    BroadcastStatus(TEXT("Epic sign-in successful."));
    ContinuePendingOperation();
}

void UEOSSessionSubsystem::ContinuePendingOperation()
{
    const EPendingOperation Operation = PendingOperation;
    PendingOperation = EPendingOperation::None;
    if (Operation == EPendingOperation::Host)
    {
        CreateSession();
    }
    else if (Operation == EPendingOperation::Find)
    {
        IOnlineSessionPtr Sessions = GetSessions();
        if (!Sessions.IsValid())
        {
            BroadcastStatus(TEXT("The online session interface is unavailable."));
            return;
        }
        SessionSearch = MakeShared<FOnlineSessionSearch>();
        SessionSearch->MaxSearchResults = 50;
        SessionSearch->bIsLanQuery = GetOnlineSubsystemName() == TEXT("NULL");
        if (GetOnlineSubsystemName() != TEXT("NULL"))
        {
            SessionSearch->QuerySettings.Set(SEARCH_LOBBIES, true, EOnlineComparisonOp::Equals);
        }
        SessionSearch->QuerySettings.Set(RazigraSessions::GameKey, true, EOnlineComparisonOp::Equals);
        FindHandle = Sessions->AddOnFindSessionsCompleteDelegate_Handle(
            FOnFindSessionsCompleteDelegate::CreateUObject(this, &ThisClass::HandleFindSessionsComplete));
        BroadcastStatus(TEXT("Searching for a two-player game..."));
        if (!Sessions->FindSessions(0, SessionSearch.ToSharedRef()))
        {
            Sessions->ClearOnFindSessionsCompleteDelegate_Handle(FindHandle);
            BroadcastStatus(TEXT("Session search could not start."));
        }
    }
}

void UEOSSessionSubsystem::CreateSession()
{
    IOnlineSessionPtr Sessions = GetSessions();
    if (!Sessions.IsValid())
    {
        BroadcastStatus(TEXT("The online session interface is unavailable."));
        return;
    }

    if (Sessions->GetNamedSession(RazigraSessions::SessionName))
    {
        DestroyHandle = Sessions->AddOnDestroySessionCompleteDelegate_Handle(
            FOnDestroySessionCompleteDelegate::CreateUObject(this, &ThisClass::HandleDestroySessionComplete));
        BroadcastStatus(TEXT("Replacing the previous session..."));
        Sessions->DestroySession(RazigraSessions::SessionName);
        return;
    }

    FOnlineSessionSettings Settings;
    Settings.NumPublicConnections = UGlobalGameData::Get(this)->RequiredPlayers;
    Settings.NumPrivateConnections = 0;
    Settings.bShouldAdvertise = true;
    Settings.bAllowJoinInProgress = true;
    Settings.bAllowJoinViaPresence = true;
    Settings.bAllowInvites = true;
    Settings.bUsesPresence = true;
    Settings.bUseLobbiesIfAvailable = true;
    Settings.bIsLANMatch = GetOnlineSubsystemName() == TEXT("NULL");
    Settings.Set(RazigraSessions::GameKey, true, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
    Settings.Set(SETTING_MAPNAME, UGlobalGameData::Get(this)->GameplayMap.ToSoftObjectPath().GetLongPackageName(),
        EOnlineDataAdvertisementType::ViaOnlineService);

    CreateHandle = Sessions->AddOnCreateSessionCompleteDelegate_Handle(
        FOnCreateSessionCompleteDelegate::CreateUObject(this, &ThisClass::HandleCreateSessionComplete));
    BroadcastStatus(FString::Printf(TEXT("Creating session using %s..."), *GetOnlineSubsystemName()));
    if (!Sessions->CreateSession(0, RazigraSessions::SessionName, Settings))
    {
        Sessions->ClearOnCreateSessionCompleteDelegate_Handle(CreateHandle);
        BroadcastStatus(TEXT("Session creation could not start."));
    }
}

void UEOSSessionSubsystem::HandleDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
    if (IOnlineSessionPtr Sessions = GetSessions())
    {
        Sessions->ClearOnDestroySessionCompleteDelegate_Handle(DestroyHandle);
    }
    if (bWasSuccessful)
    {
        CreateSession();
    }
    else
    {
        BroadcastStatus(TEXT("Could not replace the existing session."));
    }
}

void UEOSSessionSubsystem::HandleCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
    if (IOnlineSessionPtr Sessions = GetSessions())
    {
        Sessions->ClearOnCreateSessionCompleteDelegate_Handle(CreateHandle);
    }
    if (!bWasSuccessful)
    {
        BroadcastStatus(TEXT("Session creation failed."));
        return;
    }

    const UGlobalGameData* Data = UGlobalGameData::Get(this);
    bWaitingForPlayer = true;
    bSinglePlayerMode = false;
    ExpectedPlayers = Data->RequiredPlayers;
    ConnectedPlayers = FMath::Max(1, ConnectedPlayers);

    if (!Data->bWaitForAllPlayersBeforeTravel)
    {
        // Fallback flow: open the gameplay map straight away and let the other player join
        // into it. The menu stays up over the level until everybody has arrived.
        UE_LOG(LogRazigra, Log, TEXT("Host session '%s' created; opening the gameplay map immediately."),
            *SessionName.ToString());
        BroadcastStatus(FString::Printf(TEXT("Session created. Waiting for players... (%d/%d connected)"),
            ConnectedPlayers, ExpectedPlayers));
        BeginGameplayTravel(false, false);
        TravelToGameplayMap();
        return;
    }

    if (!StartListening())
    {
        bWaitingForPlayer = false;
        BroadcastStatus(TEXT("Could not open this machine for connections. Check the net driver configuration."));
        return;
    }

    UE_LOG(LogRazigra, Log, TEXT("Host session '%s' created; holding the lobby until %d players are connected."),
        *SessionName.ToString(), ExpectedPlayers);
    BroadcastStatus(FString::Printf(TEXT("Session created. Waiting for players... (%d/%d connected)"),
        ConnectedPlayers, ExpectedPlayers));
}

void UEOSSessionSubsystem::HandleFindSessionsComplete(bool bWasSuccessful)
{
    IOnlineSessionPtr Sessions = GetSessions();
    if (Sessions.IsValid())
    {
        Sessions->ClearOnFindSessionsCompleteDelegate_Handle(FindHandle);
    }
    const int32 ResultCount = SessionSearch.IsValid() ? SessionSearch->SearchResults.Num() : 0;
    UE_LOG(LogRazigra, Log, TEXT("Session search completed: success=%s, results=%d."),
        bWasSuccessful ? TEXT("true") : TEXT("false"), ResultCount);
    if (!bWasSuccessful || !SessionSearch.IsValid() || SessionSearch->SearchResults.IsEmpty())
    {
        BroadcastStatus(FString::Printf(TEXT("No joinable Zombie Zero sessions found via %s (%d results checked)."),
            *GetOnlineSubsystemName(), ResultCount));
        return;
    }

    const FOnlineSessionSearchResult* Match = SessionSearch->SearchResults.FindByPredicate(
        [](const FOnlineSessionSearchResult& Result)
        {
            bool bIsRazigra = false;
            Result.Session.SessionSettings.Get(RazigraSessions::GameKey, bIsRazigra);
            return bIsRazigra && Result.IsValid();
        });
    if (!Match)
    {
        BroadcastStatus(TEXT("Search completed, but no compatible game was found."));
        return;
    }

    UE_LOG(LogRazigra, Log, TEXT("Compatible session found: ping=%d ms, open public connections=%d."),
        Match->PingInMs, Match->Session.NumOpenPublicConnections);

    JoinHandle = Sessions->AddOnJoinSessionCompleteDelegate_Handle(
        FOnJoinSessionCompleteDelegate::CreateUObject(this, &ThisClass::HandleJoinSessionComplete));
    BroadcastStatus(TEXT("Joining game..."));
    if (!Sessions->JoinSession(0, RazigraSessions::SessionName, *Match))
    {
        Sessions->ClearOnJoinSessionCompleteDelegate_Handle(JoinHandle);
        BroadcastStatus(TEXT("Join request could not start."));
    }
}

void UEOSSessionSubsystem::HandleJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
    IOnlineSessionPtr Sessions = GetSessions();
    if (Sessions.IsValid())
    {
        Sessions->ClearOnJoinSessionCompleteDelegate_Handle(JoinHandle);
    }
    if (!Sessions.IsValid() || Result != EOnJoinSessionCompleteResult::Success)
    {
        BroadcastStatus(FString::Printf(TEXT("Joining failed (result %d)."), static_cast<int32>(Result)));
        return;
    }

    UE_LOG(LogRazigra, Log, TEXT("JoinSession completed successfully for '%s'."), *SessionName.ToString());

    FString ConnectString;
    if (!Sessions->GetResolvedConnectString(SessionName, ConnectString))
    {
        BroadcastStatus(TEXT("Joined the session but could not resolve its server address."));
        return;
    }
    if (APlayerController* Controller = GetGameInstance()->GetFirstLocalPlayerController())
    {
        BroadcastStatus(TEXT("Connected. Waiting for every player to be ready..."));
        bWaitingForPlayer = true;
        bSinglePlayerMode = false;
        ExpectedPlayers = UGlobalGameData::Get(this)->RequiredPlayers;
        // Drop the front end before travelling; it is rebuilt against the new player
        // controller on the other side, and removed for good once the match starts.
        BeginGameplayTravel(false);
        Controller->ClientTravel(ConnectString, TRAVEL_Absolute);
    }
}

void UEOSSessionSubsystem::NotifyPlayerCountChanged(int32 InConnectedPlayers, int32 InRequiredPlayers)
{
    ConnectedPlayers = InConnectedPlayers;
    ExpectedPlayers = FMath::Max(1, InRequiredPlayers);
    UE_LOG(LogRazigra, Log, TEXT("Connected player count changed: %d/%d (waiting=%s)."), ConnectedPlayers,
        ExpectedPlayers, bWaitingForPlayer ? TEXT("true") : TEXT("false"));
    if (!bWaitingForPlayer)
    {
        return;
    }

    if (ConnectedPlayers >= ExpectedPlayers)
    {
        bWaitingForPlayer = false;
        BroadcastStatus(FString::Printf(TEXT("All players connected (%d/%d). Starting Zombie Zero..."),
            ConnectedPlayers, ExpectedPlayers));
        const bool bNeedsTravel = UGlobalGameData::Get(this)->bWaitForAllPlayersBeforeTravel;
        BeginGameplayTravel(false);
        if (bNeedsTravel)
        {
            TravelToGameplayMap();
        }
    }
    else
    {
        BroadcastStatus(FString::Printf(TEXT("Waiting for another player... (%d/%d connected)"),
            ConnectedPlayers, ExpectedPlayers));
    }
}

void UEOSSessionSubsystem::ReportLobbyPopulation(int32 InConnectedPlayers, int32 InRequiredPlayers)
{
    if (!bWaitingForPlayer)
    {
        return;
    }
    const int32 Required = FMath::Max(1, InRequiredPlayers);
    if (ConnectedPlayers == InConnectedPlayers && ExpectedPlayers == Required)
    {
        return;
    }
    ConnectedPlayers = InConnectedPlayers;
    ExpectedPlayers = Required;
    if (ConnectedPlayers >= ExpectedPlayers)
    {
        BroadcastStatus(FString::Printf(TEXT("All players connected (%d/%d). Loading the map..."),
            ConnectedPlayers, ExpectedPlayers));
    }
    else
    {
        BroadcastStatus(FString::Printf(TEXT("Waiting for another player... (%d/%d connected)"),
            ConnectedPlayers, ExpectedPlayers));
    }
}

void UEOSSessionSubsystem::NotifyGameplayStarted()
{
    bWaitingForPlayer = false;
    bMainMenuRequired = false;
    if (URazigraGameInstance* RazigraInstance = Cast<URazigraGameInstance>(GetGameInstance()))
    {
        RazigraInstance->HideMainMenu();
    }
}

void UEOSSessionSubsystem::HandleNetworkFailure(UWorld* World, UNetDriver* NetDriver,
    ENetworkFailure::Type FailureType, const FString& Error)
{
    BroadcastStatus(FString::Printf(TEXT("Network failure [%d]: %s"), static_cast<int32>(FailureType), *Error));
}

void UEOSSessionSubsystem::HandleTravelFailure(UWorld* World, ETravelFailure::Type FailureType,
    const FString& Error)
{
    BroadcastStatus(FString::Printf(TEXT("Travel failure [%d]: %s"), static_cast<int32>(FailureType), *Error));
}

void UEOSSessionSubsystem::CancelSearch()
{
    if (IOnlineSessionPtr Sessions = GetSessions())
    {
        if (SessionSearch.IsValid() && SessionSearch->SearchState == EOnlineAsyncTaskState::InProgress)
        {
            Sessions->CancelFindSessions();
        }
        Sessions->ClearOnFindSessionsCompleteDelegate_Handle(FindHandle);
    }
    SessionSearch.Reset();
}
