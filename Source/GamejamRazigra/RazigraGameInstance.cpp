#include "RazigraGameInstance.h"

#include "CoopMenuWidget.h"
#include "EOSSessionSubsystem.h"
#include "GameFramework/PlayerController.h"
#include "GamejamRazigra.h"
#include "GlobalGameData.h"
#include "TimerManager.h"

void URazigraGameInstance::OnStart()
{
    Super::OnStart();
    if (GetWorld() && GetWorld()->GetNetMode() != NM_DedicatedServer)
    {
        ShowMainMenu();
    }
}

void URazigraGameInstance::ShowMainMenu()
{
    if (MainMenuWidget || !GetWorld() || GetWorld()->GetNetMode() == NM_DedicatedServer)
    {
        return;
    }

    APlayerController* Controller = GetFirstLocalPlayerController();
    if (!Controller)
    {
        ShowMenuRetryTimer = GetWorld()->GetTimerManager().SetTimerForNextTick(this, &ThisClass::ShowMainMenu);
        return;
    }

    const UGlobalGameData* Data = UGlobalGameData::Get(this);
    TSubclassOf<UCoopMenuWidget> MenuClass = Data->MainMenuWidgetClass;
    if (!MenuClass)
    {
        MenuClass = UCoopMenuWidget::StaticClass();
    }
    MainMenuWidget = CreateWidget<UCoopMenuWidget>(Controller, MenuClass);
    if (!MainMenuWidget)
    {
        return;
    }

    MainMenuWidget->AddToViewport(100);
    UE_LOG(LogRazigra, Log, TEXT("Main menu displayed over startup map %s."), *GetWorld()->GetMapName());
    Controller->bShowMouseCursor = true;
    FInputModeUIOnly InputMode;
    InputMode.SetWidgetToFocus(MainMenuWidget->TakeWidget());
    Controller->SetInputMode(InputMode);
}

void URazigraGameInstance::HideMainMenu()
{
    if (GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(ShowMenuRetryTimer);
    }
    if (MainMenuWidget)
    {
        MainMenuWidget->RemoveFromParent();
        MainMenuWidget = nullptr;
    }

    if (APlayerController* Controller = GetFirstLocalPlayerController())
    {
        Controller->bShowMouseCursor = false;
        Controller->SetInputMode(FInputModeGameOnly());
    }
}

bool URazigraGameInstance::IsMainMenuVisible() const
{
    return IsValid(MainMenuWidget.Get());
}
