#include "CoopMenuWidget.h"

#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "EOSSessionSubsystem.h"
#include "Engine/GameInstance.h"
#include "Blueprint/WidgetTree.h"
#include "Styling/CoreStyle.h"

UCoopMenuWidget::UCoopMenuWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
    , TitleFont(FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), 46))
    , ButtonFont(FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), 16))
    , StatusFont(FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 12))
    , BackdropColor(0.0f, 0.0f, 0.0f, 1.0f)
    , TitleColor(0.82f, 0.10f, 0.06f, 1.0f)
{
}

void UCoopMenuWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    SetIsFocusable(true);
    if (WidgetTree->RootWidget)
    {
        return;
    }

    // Pitch black, edge to edge, with the whole menu centred inside it.
    UBorder* Backdrop = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Backdrop"));
    Backdrop->SetBrushColor(BackdropColor);
    Backdrop->SetPadding(FMargin(0.0f));
    Backdrop->SetHorizontalAlignment(HAlign_Center);
    Backdrop->SetVerticalAlignment(VAlign_Center);
    WidgetTree->RootWidget = Backdrop;

    USizeBox* Column = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("MenuColumn"));
    Column->SetWidthOverride(360.0f);
    Backdrop->SetContent(Column);

    UVerticalBox* Layout = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MenuLayout"));
    Column->SetContent(Layout);

    UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Title"));
    Title->SetText(FText::FromString(TEXT("ZOMBIE ZERO")));
    Title->SetColorAndOpacity(FSlateColor(TitleColor));
    Title->SetJustification(ETextJustify::Center);
    Title->SetFont(TitleFont);
    if (UVerticalBoxSlot* TitleSlot = Layout->AddChildToVerticalBox(Title))
    {
        TitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 34.0f));
    }

    HostButton = BuildButton(TEXT("Host"), TEXT("HOST"));
    JoinButton = BuildButton(TEXT("Join"), TEXT("JOIN"));
    SinglePlayerButton = BuildButton(TEXT("Solo"), TEXT("SOLO"));
    for (UButton* Button : { HostButton.Get(), JoinButton.Get(), SinglePlayerButton.Get() })
    {
        if (UVerticalBoxSlot* ButtonSlot = Layout->AddChildToVerticalBox(Button))
        {
            ButtonSlot->SetPadding(FMargin(0.0f, 5.0f));
        }
    }

    StatusText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("StatusText"));
    StatusText->SetText(FText::GetEmpty());
    StatusText->SetJustification(ETextJustify::Center);
    StatusText->SetAutoWrapText(true);
    StatusText->SetColorAndOpacity(FSlateColor(FLinearColor(0.55f, 0.58f, 0.62f, 1.0f)));
    StatusText->SetFont(StatusFont);
    if (UVerticalBoxSlot* StatusSlot = Layout->AddChildToVerticalBox(StatusText))
    {
        StatusSlot->SetPadding(FMargin(0.0f, 28.0f, 0.0f, 0.0f));
    }

    HostButton->OnClicked.AddDynamic(this, &ThisClass::HandleHostClicked);
    JoinButton->OnClicked.AddDynamic(this, &ThisClass::HandleJoinClicked);
    SinglePlayerButton->OnClicked.AddDynamic(this, &ThisClass::HandleSinglePlayerClicked);
}

UButton* UCoopMenuWidget::BuildButton(const FString& Tag, const FString& Label)
{
    UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), *(Tag + TEXT("Button")));
    UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), *(Tag + TEXT("Label")));
    Text->SetText(FText::FromString(Label));
    Text->SetJustification(ETextJustify::Center);
    Text->SetFont(ButtonFont);
    Button->SetContent(Text);
    return Button;
}

void UCoopMenuWidget::NativeConstruct()
{
    Super::NativeConstruct();
    if (UEOSSessionSubsystem* Sessions = GetGameInstance()->GetSubsystem<UEOSSessionSubsystem>())
    {
        Sessions->OnStatusChanged.AddUniqueDynamic(this, &ThisClass::HandleStatusChanged);
        ApplyLobbyMode();
        HandleStatusChanged(Sessions->GetLastStatus());
    }
}

void UCoopMenuWidget::ApplyLobbyMode()
{
    const UEOSSessionSubsystem* Sessions = GetGameInstance()
        ? GetGameInstance()->GetSubsystem<UEOSSessionSubsystem>() : nullptr;
    if (!Sessions || !Sessions->IsWaitingForPlayers())
    {
        return;
    }

    // Nothing left to click once the session is up: the status line carries the wait.
    for (UButton* Button : { HostButton.Get(), JoinButton.Get(), SinglePlayerButton.Get() })
    {
        if (Button)
        {
            Button->SetVisibility(ESlateVisibility::Collapsed);
        }
    }
}

void UCoopMenuWidget::NativeDestruct()
{
    if (GetGameInstance())
    {
        if (UEOSSessionSubsystem* Sessions = GetGameInstance()->GetSubsystem<UEOSSessionSubsystem>())
        {
            Sessions->OnStatusChanged.RemoveDynamic(this, &ThisClass::HandleStatusChanged);
        }
    }
    Super::NativeDestruct();
}

void UCoopMenuWidget::HandleHostClicked()
{
    GetGameInstance()->GetSubsystem<UEOSSessionSubsystem>()->HostGame();
}

void UCoopMenuWidget::HandleJoinClicked()
{
    GetGameInstance()->GetSubsystem<UEOSSessionSubsystem>()->FindAndJoinGame();
}

void UCoopMenuWidget::HandleSinglePlayerClicked()
{
    GetGameInstance()->GetSubsystem<UEOSSessionSubsystem>()->StartSinglePlayer();
}

void UCoopMenuWidget::HandleStatusChanged(const FString& NewStatus)
{
    // Only ever the latest line: the running log belongs in the output log, not on screen.
    if (StatusText)
    {
        StatusText->SetText(FText::FromString(NewStatus));
    }
    ApplyLobbyMode();

    const bool bBusy = NewStatus.Contains(TEXT("..."));
    for (UButton* Button : { HostButton.Get(), JoinButton.Get(), SinglePlayerButton.Get() })
    {
        if (Button)
        {
            Button->SetIsEnabled(!bBusy);
        }
    }
}
