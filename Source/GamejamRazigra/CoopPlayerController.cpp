#include "CoopPlayerController.h"

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
void ACoopPlayerController::LookX(float Value) { PendingLookInput.X = Value; }
void ACoopPlayerController::LookY(float Value) { PendingLookInput.Y = Value; }

void ACoopPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ACoopPlayerController, PlayerSlot);
}
