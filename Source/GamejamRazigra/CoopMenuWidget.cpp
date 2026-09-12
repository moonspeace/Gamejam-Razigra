#include "CoopMenuWidget.h"

#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "EOSSessionSubsystem.h"
#include "Engine/GameInstance.h"
#include "Blueprint/WidgetTree.h"
#include "Styling/CoreStyle.h"

UCoopMenuWidget::UCoopMenuWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
    , TitleFont(FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), 44))
    , SubtitleFont(FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 18))
    , ButtonFont(FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), 18))
    , StatusFont(FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 14))
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

    UBorder* Backdrop = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Backdrop"));
    Backdrop->SetBrushColor(FLinearColor(0.015f, 0.02f, 0.025f, 0.96f));
    Backdrop->SetPadding(FMargin(48.0f));
    WidgetTree->RootWidget = Backdrop;

    UVerticalBox* Layout = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MenuLayout"));
    Backdrop->SetContent(Layout);

    UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Title"));
    Title->SetText(FText::FromString(TEXT("ZOMBIE ZERO")));
    Title->SetColorAndOpacity(FSlateColor(FLinearColor(0.8f, 0.08f, 0.04f)));
    Title->SetJustification(ETextJustify::Center);
    Title->SetFont(TitleFont);
    Layout->AddChildToVerticalBox(Title)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 28.0f));

    UTextBlock* Subtitle = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Subtitle"));
    Subtitle->SetText(FText::FromString(TEXT("TWO PLAYERS. ONE SURVIVOR.\nEvery action requires both players.")));
    Subtitle->SetJustification(ETextJustify::Center);
    Subtitle->SetColorAndOpacity(FSlateColor(FLinearColor::White));
    Subtitle->SetFont(SubtitleFont);
    Layout->AddChildToVerticalBox(Subtitle)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 28.0f));

    HostButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("HostButton"));
    UTextBlock* HostLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("HostLabel"));
    HostLabel->SetText(FText::FromString(TEXT("HOST GAME")));
    HostLabel->SetJustification(ETextJustify::Center);
    HostLabel->SetFont(ButtonFont);
    HostButton->SetContent(HostLabel);
    Layout->AddChildToVerticalBox(HostButton)->SetPadding(FMargin(0.0f, 6.0f));

    JoinButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("JoinButton"));
    UTextBlock* JoinLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("JoinLabel"));
    JoinLabel->SetText(FText::FromString(TEXT("FIND & JOIN GAME")));
    JoinLabel->SetJustification(ETextJustify::Center);
    JoinLabel->SetFont(ButtonFont);
    JoinButton->SetContent(JoinLabel);
    Layout->AddChildToVerticalBox(JoinButton)->SetPadding(FMargin(0.0f, 6.0f));

    SinglePlayerButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("SinglePlayerButton"));
    UTextBlock* SinglePlayerLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SinglePlayerLabel"));
    SinglePlayerLabel->SetText(FText::FromString(TEXT("SINGLE PLAYER (TEST)")));
    SinglePlayerLabel->SetJustification(ETextJustify::Center);
    SinglePlayerLabel->SetFont(ButtonFont);
    SinglePlayerButton->SetContent(SinglePlayerLabel);
    Layout->AddChildToVerticalBox(SinglePlayerButton)->SetPadding(FMargin(0.0f, 6.0f));

    StatusText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("StatusText"));
    StatusText->SetText(FText::FromString(TEXT("Ready")));
    StatusText->SetJustification(ETextJustify::Center);
    StatusText->SetAutoWrapText(true);
    StatusText->SetFont(StatusFont);
    Layout->AddChildToVerticalBox(StatusText)->SetPadding(FMargin(0.0f, 24.0f, 0.0f, 0.0f));

    HostButton->OnClicked.AddDynamic(this, &ThisClass::HandleHostClicked);
    JoinButton->OnClicked.AddDynamic(this, &ThisClass::HandleJoinClicked);
    SinglePlayerButton->OnClicked.AddDynamic(this, &ThisClass::HandleSinglePlayerClicked);
}

void UCoopMenuWidget::NativeConstruct()
{
    Super::NativeConstruct();
    if (UEOSSessionSubsystem* Sessions = GetGameInstance()->GetSubsystem<UEOSSessionSubsystem>())
    {
        Sessions->OnStatusChanged.AddUniqueDynamic(this, &ThisClass::HandleStatusChanged);
        HandleStatusChanged(FString::Printf(TEXT("Online service: %s"), *Sessions->GetOnlineSubsystemName()));
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
    HostButton->SetIsEnabled(false);
    JoinButton->SetIsEnabled(false);
    SinglePlayerButton->SetIsEnabled(false);
    GetGameInstance()->GetSubsystem<UEOSSessionSubsystem>()->HostGame();
}

void UCoopMenuWidget::HandleJoinClicked()
{
    HostButton->SetIsEnabled(false);
    JoinButton->SetIsEnabled(false);
    SinglePlayerButton->SetIsEnabled(false);
    GetGameInstance()->GetSubsystem<UEOSSessionSubsystem>()->FindAndJoinGame();
}

void UCoopMenuWidget::HandleSinglePlayerClicked()
{
    HostButton->SetIsEnabled(false);
    JoinButton->SetIsEnabled(false);
    SinglePlayerButton->SetIsEnabled(false);
    GetGameInstance()->GetSubsystem<UEOSSessionSubsystem>()->StartSinglePlayer();
}

void UCoopMenuWidget::HandleStatusChanged(const FString& NewStatus)
{
    if (StatusText)
    {
        StatusText->SetText(FText::FromString(NewStatus));
    }
    const bool bBusy = NewStatus.Contains(TEXT("..."));
    if (HostButton)
    {
        HostButton->SetIsEnabled(!bBusy);
    }
    if (JoinButton)
    {
        JoinButton->SetIsEnabled(!bBusy);
    }
    if (SinglePlayerButton)
    {
        SinglePlayerButton->SetIsEnabled(!bBusy);
    }
}
