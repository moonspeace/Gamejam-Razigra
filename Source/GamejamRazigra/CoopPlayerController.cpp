#include "CoopPlayerController.h"

#include "Camera/PlayerCameraManager.h"
#include "CoopGameState.h"
#include "CoopHudWidget.h"
#include "EOSSessionSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GlobalGameData.h"
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

    if (IsLocalController() && !PendingLookInput.IsNearlyZero())
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
    SetViewTarget(Hero);
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
    ServerRequestRestartRun();
}

void ACoopPlayerController::ServerRequestRestartRun_Implementation()
{
    if (!GetWorld())
    {
        return;
    }
    const FString Map = UGlobalGameData::Get(this)->GameplayMap.ToSoftObjectPath().GetLongPackageName();
    const bool bStandalone = GetWorld()->GetNetMode() == NM_Standalone;
    GetWorld()->ServerTravel(Map + (bStandalone ? TEXT("") : TEXT("?listen")));
}

void ACoopPlayerController::ClientBindToSharedHero_Implementation(ASharedHeroCharacter* Hero)
{
    BindToSharedHero(Hero);
}

void ACoopPlayerController::SetAction(EConsensusAction Action, bool bPressed)
{
    if (PlayerSlot != INDEX_NONE)
    {
        ServerSetAction(Action, bPressed);
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
void ACoopPlayerController::LookX(float Value) { PendingLookInput.X = Value; }
void ACoopPlayerController::LookY(float Value) { PendingLookInput.Y = Value; }

void ACoopPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ACoopPlayerController, PlayerSlot);
    DOREPLIFETIME(ACoopPlayerController, SoloAbilityRole);
}
