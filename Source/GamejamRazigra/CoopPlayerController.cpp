#include "CoopPlayerController.h"

#include "Camera/PlayerCameraManager.h"
#include "CoopGameState.h"
#include "CoopHudWidget.h"
#include "EOSSessionSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GlobalGameData.h"
#include "GamejamRazigra.h"
#include "HologramPuzzle.h"
#include "Net/UnrealNetwork.h"
#include "RazigraGameInstance.h"

ACoopPlayerController::ACoopPlayerController()
{
    bAutoManageActiveCameraTarget = false;
    PrimaryActorTick.bCanEverTick = true;
}

void ACoopPlayerController::BeginPlay()
{
    Super::BeginPlay();
    if (IsLocalController())
    {
        const UGameInstance* Instance = GetGameInstance();
        const UEOSSessionSubsystem* Sessions = Instance
            ? Instance->GetSubsystem<UEOSSessionSubsystem>() : nullptr;
        if (Sessions && Sessions->IsSinglePlayerMode())
        {
            // Solo OpenLevel can recreate this controller before the game mode has spawned and
            // bound the hero. Never apply the multiplayer waiting blackout to that short gap.
            if (PlayerCameraManager)
            {
                PlayerCameraManager->SetManualCameraFade(0.0f, FLinearColor::Black, false);
            }
        }
        else
        {
            // Multiplayer deliberately remains black while the other player is connecting.
            HoldScreenBlack();
        }
    }
}

void ACoopPlayerController::HoldScreenBlack()
{
    if (PlayerCameraManager)
    {
        PlayerCameraManager->SetManualCameraFade(1.0f, FLinearColor::Black, false);
    }
}

void ACoopPlayerController::FadeScreenIn()
{
    if (PlayerCameraManager)
    {
        PlayerCameraManager->StartCameraFade(1.0f, 0.0f,
            UGlobalGameData::Get(this)->GameplayFadeInSeconds, FLinearColor::Black, false, false);
    }
}

void ACoopPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    HideGameplayHud();
    Super::EndPlay(EndPlayReason);
}

void ACoopPlayerController::PlayerTick(float DeltaTime)
{
    Super::PlayerTick(DeltaTime);

    if (IsLocalController() && !ActivePuzzle && !PendingLookInput.IsNearlyZero())
    {
        ServerSubmitLook(PendingLookInput);
        PendingLookInput = FVector2D::ZeroVector;
    }

    if (IsLocalController() && !SharedHero)
    {
        SharedHeroSearchTime -= DeltaTime;
        if (SharedHeroSearchTime <= 0.0f)
        {
            SharedHeroSearchTime = 0.25f;
            if (const ACoopGameState* State = GetWorld()->GetGameState<ACoopGameState>())
            {
                BindToSharedHero(State->SharedHero);
            }
        }
    }
}

void ACoopPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();
    InputComponent->BindAction(TEXT("MoveForward"), IE_Pressed, this, &ThisClass::MoveForwardPressed);
    InputComponent->BindAction(TEXT("MoveForward"), IE_Released, this, &ThisClass::MoveForwardReleased);
    InputComponent->BindAction(TEXT("MoveBackward"), IE_Pressed, this, &ThisClass::MoveBackwardPressed);
    InputComponent->BindAction(TEXT("MoveBackward"), IE_Released, this, &ThisClass::MoveBackwardReleased);
    InputComponent->BindAction(TEXT("MoveLeft"), IE_Pressed, this, &ThisClass::MoveLeftPressed);
    InputComponent->BindAction(TEXT("MoveLeft"), IE_Released, this, &ThisClass::MoveLeftReleased);
    InputComponent->BindAction(TEXT("MoveRight"), IE_Pressed, this, &ThisClass::MoveRightPressed);
    InputComponent->BindAction(TEXT("MoveRight"), IE_Released, this, &ThisClass::MoveRightReleased);
    InputComponent->BindAction(TEXT("ConsensusJump"), IE_Pressed, this, &ThisClass::JumpPressed);
    InputComponent->BindAction(TEXT("ConsensusJump"), IE_Released, this, &ThisClass::JumpReleased);
    InputComponent->BindAction(TEXT("ConsensusCrouch"), IE_Pressed, this, &ThisClass::CrouchPressed);
    InputComponent->BindAction(TEXT("ConsensusCrouch"), IE_Released, this, &ThisClass::CrouchReleased);
    InputComponent->BindAction(TEXT("ConsensusFire"), IE_Pressed, this, &ThisClass::FirePressed);
    InputComponent->BindAction(TEXT("ConsensusFire"), IE_Released, this, &ThisClass::FireReleased);
    InputComponent->BindAction(TEXT("RoleAbility"), IE_Pressed, this, &ThisClass::AbilityPressed);
    InputComponent->BindAction(TEXT("RoleAbility"), IE_Released, this, &ThisClass::AbilityReleased);
    InputComponent->BindAction(TEXT("SwitchSoloRole"), IE_Pressed, this, &ThisClass::SwitchSoloRole);
    InputComponent->BindAction(TEXT("Interact"), IE_Pressed, this, &ThisClass::InteractPressed);
    InputComponent->BindAxis(TEXT("ConsensusLookX"), this, &ThisClass::LookX);
    InputComponent->BindAxis(TEXT("ConsensusLookY"), this, &ThisClass::LookY);
}

void ACoopPlayerController::SetPlayerSlot(int32 NewSlot)
{
    if (HasAuthority())
    {
        PlayerSlot = NewSlot;
        OnRep_PlayerSlot();
    }
}

int32 ACoopPlayerController::GetDisplayedRole() const
{
    const UEOSSessionSubsystem* Sessions = GetGameInstance()
        ? GetGameInstance()->GetSubsystem<UEOSSessionSubsystem>() : nullptr;
    return Sessions && Sessions->IsSinglePlayerMode() ? SoloAbilityRole : PlayerSlot;
}

void ACoopPlayerController::OnRep_PlayerSlot()
{
}

void ACoopPlayerController::BindToSharedHero(ASharedHeroCharacter* Hero)
{
    if (!Hero || !IsLocalController())
    {
        return;
    }
    SharedHero = Hero;
    if (!ActivePuzzle) SetViewTarget(Hero);
    bShowMouseCursor = false;
    SetInputMode(FInputModeGameOnly());

    // The shared hero only exists once every player is in, so this is the moment the
    // match actually starts: drop the front end and raise the in-game consensus HUD.
    if (UGameInstance* Instance = GetGameInstance())
    {
        if (UEOSSessionSubsystem* Sessions = Instance->GetSubsystem<UEOSSessionSubsystem>())
        {
            Sessions->NotifyGameplayStarted();
        }
        if (URazigraGameInstance* RazigraInstance = Cast<URazigraGameInstance>(Instance))
        {
            RazigraInstance->HideMainMenu();
        }
    }
    ShowGameplayHud();
    FadeScreenIn();
}

void ACoopPlayerController::ShowGameplayHud()
{
    if (GameplayHud || !IsLocalController())
    {
        return;
    }
    TSubclassOf<UCoopHudWidget> HudClass = UGlobalGameData::Get(this)->GameplayHudWidgetClass;
    if (!HudClass)
    {
        HudClass = UCoopHudWidget::StaticClass();
    }
    GameplayHud = CreateWidget<UCoopHudWidget>(this, HudClass);
    if (GameplayHud)
    {
        GameplayHud->AddToViewport(10);
    }
}

void ACoopPlayerController::HideGameplayHud()
{
    if (GameplayHud)
    {
        GameplayHud->RemoveFromParent();
        GameplayHud = nullptr;
    }
}

void ACoopPlayerController::HandleHeroDamaged(float DamageAmount)
{
    if (!IsLocalController())
    {
        return;
    }
    if (!GameplayHud)
    {
        ShowGameplayHud();
    }
    if (GameplayHud)
    {
        GameplayHud->ShowDamageFeedback(DamageAmount);
    }

    const UGlobalGameData* Data = UGlobalGameData::Get(this);
    if (PlayerCameraManager && Data->HeroDamageCameraShakeClass && Data->HeroDamageCameraShakeScale > 0.0f)
    {
        PlayerCameraManager->StartCameraShake(Data->HeroDamageCameraShakeClass,
            Data->HeroDamageCameraShakeScale);
    }
}

void ACoopPlayerController::HandleGunFired(bool bHit)
{
    if (IsLocalController() && GameplayHud)
    {
        GameplayHud->ShowFireFeedback(bHit);
    }
}

void ACoopPlayerController::RequestRestartRun()
{
    if (!CanRestartRun())
    {
        return;
    }
    ServerRequestRestartRun();
}

bool ACoopPlayerController::CanRestartRun() const
{
    if (!IsLocalController() || !GetWorld())
    {
        return false;
    }
    // Slot zero is allocated to the listen host. Unlike HasAuthority(), this remains reliable
    // for local PIE clients and becomes available as soon as PlayerSlot finishes replicating.
    return GetWorld()->GetNetMode() == NM_Standalone || PlayerSlot == 0;
}

void ACoopPlayerController::ServerRequestRestartRun_Implementation()
{
    if (!GetWorld())
    {
        return;
    }
    // Validate by the replicated/authoritative slot here; IsLocalController is intentionally not
    // used because an RPC's server-side controller copy need not be local in every PIE topology.
    if (GetWorld()->GetNetMode() != NM_Standalone && PlayerSlot != 0)
    {
        UE_LOG(LogRazigra, Warning, TEXT("Rejected reload request from non-host player slot %d."), PlayerSlot);
        return;
    }

    UE_LOG(LogRazigra, Log, TEXT("Host requested synchronized run reload for all players."));
    // ?Restart tells the authoritative world to reload its current URL. This is the same
    // low-level path used by AGameMode::RestartGame, but it also works with AGameModeBase and
    // keeps every connected client attached to the server travel.
    if (!GetWorld()->ServerTravel(TEXT("?Restart"), false))
    {
        UE_LOG(LogRazigra, Error, TEXT("Synchronized run reload could not start."));
    }
}

void ACoopPlayerController::ClientBindToSharedHero_Implementation(ASharedHeroCharacter* Hero)
{
    BindToSharedHero(Hero);
}

void ACoopPlayerController::SetAction(EConsensusAction Action, bool bPressed)
{
    if (ActivePuzzle)
    {
        if (!bPressed) return;
        switch (Action)
        {
        case EConsensusAction::MoveForward:  ServerPuzzleInput(EPuzzleDirection::North, false); break;
        case EConsensusAction::MoveBackward: ServerPuzzleInput(EPuzzleDirection::South, false); break;
        case EConsensusAction::MoveLeft:     ServerPuzzleInput(EPuzzleDirection::West, false); break;
        case EConsensusAction::MoveRight:    ServerPuzzleInput(EPuzzleDirection::East, false); break;
        case EConsensusAction::Jump:         ServerPuzzleInput(EPuzzleDirection::North, true); break;
        default: break;
        }
        return;
    }
    if (PlayerSlot != INDEX_NONE)
    {
        ServerSetAction(Action, bPressed);
    }
}

void ACoopPlayerController::SetPuzzleFocus(AHologramPuzzle* Puzzle)
{
    // The puzzle republishes its focus on every board change, so ignore repeats: without this
    // each cursor move would restart the camera blend and the view would never settle.
    if (!IsLocalController() || ActivePuzzle == Puzzle) return;
    ActivePuzzle = Puzzle;
    if (GameplayHud) GameplayHud->SetVisibility(Puzzle ? ESlateVisibility::Collapsed : ESlateVisibility::SelfHitTestInvisible);
    if (Puzzle) SetViewTargetWithBlend(Puzzle, Puzzle->ViewBlendTime, EViewTargetBlendFunction::VTBlend_Cubic);
    else if (SharedHero) SetViewTargetWithBlend(SharedHero, 0.55f, EViewTargetBlendFunction::VTBlend_Cubic);
}

void ACoopPlayerController::ClientSetPuzzleFocus_Implementation(AHologramPuzzle* Puzzle)
{
    SetPuzzleFocus(Puzzle);
}

void ACoopPlayerController::ServerTogglePuzzleInteraction_Implementation()
{
    if (!GetWorld()) return;
    for (TActorIterator<AHologramPuzzle> It(GetWorld()); It; ++It)
    {
        if (It->IsFocused())
        {
            // The puzzle tells every local player itself, through its replicated focus.
            It->SetFocused(false);
            return;
        }
    }
    const ACoopGameState* State = GetWorld()->GetGameState<ACoopGameState>();
    ASharedHeroCharacter* Hero = State ? State->SharedHero : nullptr;
    AHologramPuzzle* Nearest = nullptr;
    float Best = TNumericLimits<float>::Max();
    for (TActorIterator<AHologramPuzzle> It(GetWorld()); It; ++It)
    {
        if (!It->IsWithinInteractionRange(Hero) || It->IsSolved()) continue;
        const float Distance = FVector::DistSquared(It->GetActorLocation(), Hero->GetActorLocation());
        if (Distance < Best) { Best = Distance; Nearest = *It; }
    }
    if (Nearest)
    {
        Hero->ResetParticipant(0); Hero->ResetParticipant(1);
        Nearest->SetFocused(true);
    }
}

void ACoopPlayerController::ServerPuzzleInput_Implementation(EPuzzleDirection Direction, bool bActivate)
{
    // A standalone solo controller may issue its first puzzle input before PlayerSlot has
    // replicated. Treat it as participant zero; the actor itself still enforces multiplayer.
    const int32 PuzzleParticipant = PlayerSlot == INDEX_NONE ? 0 : PlayerSlot;
    EPuzzleInput Input = EPuzzleInput::Activate;
    if (!bActivate)
    {
        switch (Direction)
        {
        case EPuzzleDirection::North: Input = EPuzzleInput::MoveNorth; break;
        case EPuzzleDirection::East:  Input = EPuzzleInput::MoveEast;  break;
        case EPuzzleDirection::South: Input = EPuzzleInput::MoveSouth; break;
        default:                      Input = EPuzzleInput::MoveWest;  break;
        }
    }
    for (TActorIterator<AHologramPuzzle> It(GetWorld()); It; ++It)
    {
        if (!It->IsFocused()) continue;
        // The board waits for both players, so hand it the slot that asked.
        It->SubmitInput(PuzzleParticipant, Input);
        return;
    }
}

void ACoopPlayerController::ServerSetAction_Implementation(EConsensusAction Action, bool bPressed)
{
    if (PlayerSlot == INDEX_NONE || static_cast<uint8>(Action) >= static_cast<uint8>(EConsensusAction::MAX))
    {
        return;
    }
    if (const ACoopGameState* State = GetWorld()->GetGameState<ACoopGameState>())
    {
        if (State->SharedHero)
        {
            State->SharedHero->SetParticipantAction(PlayerSlot, Action, bPressed);
        }
    }
}

void ACoopPlayerController::ServerSubmitLook_Implementation(FVector2D LookDelta)
{
    if (PlayerSlot == INDEX_NONE || LookDelta.ContainsNaN())
    {
        return;
    }
    if (const ACoopGameState* State = GetWorld()->GetGameState<ACoopGameState>())
    {
        if (State->SharedHero)
        {
            State->SharedHero->SubmitParticipantLook(PlayerSlot, LookDelta);
        }
    }
}

void ACoopPlayerController::ServerSetRoleAbility_Implementation(bool bPressed)
{
    if (PlayerSlot != INDEX_NONE)
    {
        if (const ACoopGameState* State = GetWorld()->GetGameState<ACoopGameState>())
        {
            if (State->SharedHero)
            {
                const UEOSSessionSubsystem* Sessions = GetGameInstance()
                    ? GetGameInstance()->GetSubsystem<UEOSSessionSubsystem>() : nullptr;
                const int32 AbilityRole = Sessions && Sessions->IsSinglePlayerMode()
                    ? SoloAbilityRole : PlayerSlot;
                State->SharedHero->SetParticipantAbility(AbilityRole, bPressed);
            }
        }
    }
}

void ACoopPlayerController::ServerSwitchSoloRole_Implementation()
{
    const UEOSSessionSubsystem* Sessions = GetGameInstance()
        ? GetGameInstance()->GetSubsystem<UEOSSessionSubsystem>() : nullptr;
    if (!Sessions || !Sessions->IsSinglePlayerMode() || PlayerSlot == INDEX_NONE)
    {
        return;
    }
    if (const ACoopGameState* State = GetWorld()->GetGameState<ACoopGameState>())
    {
        if (State->SharedHero)
        {
            State->SharedHero->SetParticipantAbility(SoloAbilityRole, false);
        }
    }
    SoloAbilityRole = SoloAbilityRole == 0 ? 1 : 0;
    ForceNetUpdate();
}

void ACoopPlayerController::MoveForwardPressed() { SetAction(EConsensusAction::MoveForward, true); }
void ACoopPlayerController::MoveForwardReleased() { SetAction(EConsensusAction::MoveForward, false); }
void ACoopPlayerController::MoveBackwardPressed() { SetAction(EConsensusAction::MoveBackward, true); }
void ACoopPlayerController::MoveBackwardReleased() { SetAction(EConsensusAction::MoveBackward, false); }
void ACoopPlayerController::MoveLeftPressed() { SetAction(EConsensusAction::MoveLeft, true); }
void ACoopPlayerController::MoveLeftReleased() { SetAction(EConsensusAction::MoveLeft, false); }
void ACoopPlayerController::MoveRightPressed() { SetAction(EConsensusAction::MoveRight, true); }
void ACoopPlayerController::MoveRightReleased() { SetAction(EConsensusAction::MoveRight, false); }
void ACoopPlayerController::JumpPressed() { SetAction(EConsensusAction::Jump, true); }
void ACoopPlayerController::JumpReleased() { SetAction(EConsensusAction::Jump, false); }
void ACoopPlayerController::CrouchPressed() { SetAction(EConsensusAction::Crouch, true); }
void ACoopPlayerController::CrouchReleased() { SetAction(EConsensusAction::Crouch, false); }
void ACoopPlayerController::FirePressed() { SetAction(EConsensusAction::Fire, true); }
void ACoopPlayerController::FireReleased() { SetAction(EConsensusAction::Fire, false); }
void ACoopPlayerController::AbilityPressed() { ServerSetRoleAbility(true); }
void ACoopPlayerController::AbilityReleased() { ServerSetRoleAbility(false); }
void ACoopPlayerController::SwitchSoloRole() { ServerSwitchSoloRole(); }
void ACoopPlayerController::InteractPressed() { ServerTogglePuzzleInteraction(); }
void ACoopPlayerController::LookX(float Value) { PendingLookInput.X = Value; }
void ACoopPlayerController::LookY(float Value) { PendingLookInput.Y = Value; }

void ACoopPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ACoopPlayerController, PlayerSlot);
    DOREPLIFETIME(ACoopPlayerController, SoloAbilityRole);
}
