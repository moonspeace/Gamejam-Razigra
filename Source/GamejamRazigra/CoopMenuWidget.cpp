#include "CoopMenuWidget.h"

#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "EOSSessionSubsystem.h"
#include "Engine/GameInstance.h"
#include "GlobalGameData.h"
#include "Blueprint/WidgetTree.h"

UCoopMenuWidget::UCoopMenuWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
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

    TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Title"));
    TitleText->SetText(FText::FromString(TEXT("ZOMBIE ZERO")));
    TitleText->SetColorAndOpacity(FSlateColor(TitleColor));
    TitleText->SetJustification(ETextJustify::Center);
    TitleText->SetFont(UGlobalGameData::Get(this)->MenuTitleFont);
    TitleText->SetShadowOffset(FVector2D(3.0f, 3.0f));
    TitleText->SetShadowColorAndOpacity(FLinearColor(0.25f, 0.0f, 0.0f, 0.8f));
    if (UVerticalBoxSlot* TitleSlot = Layout->AddChildToVerticalBox(TitleText))
    {
        TitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 34.0f));
    }

    HostButton = BuildButton(TEXT("Host"), TEXT("HOST"));
    JoinButton = BuildButton(TEXT("Join"), TEXT("JOIN"));
    SinglePlayerButton = BuildButton(TEXT("Solo"), TEXT("SOLO"));
    MenuButtons = { HostButton, JoinButton, SinglePlayerButton };
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
    StatusText->SetFont(UGlobalGameData::Get(this)->MenuStatusFont);
    if (UVerticalBoxSlot* StatusSlot = Layout->AddChildToVerticalBox(StatusText))
    {
        StatusSlot->SetPadding(FMargin(0.0f, 28.0f, 0.0f, 0.0f));
    }

    HostButton->OnClicked.AddDynamic(this, &ThisClass::HandleHostClicked);
    JoinButton->OnClicked.AddDynamic(this, &ThisClass::HandleJoinClicked);
    SinglePlayerButton->OnClicked.AddDynamic(this, &ThisClass::HandleSinglePlayerClicked);
    HostButton->OnHovered.AddDynamic(this, &ThisClass::HandleHostHovered);
    JoinButton->OnHovered.AddDynamic(this, &ThisClass::HandleJoinHovered);
    SinglePlayerButton->OnHovered.AddDynamic(this, &ThisClass::HandleSoloHovered);
    HostButton->OnUnhovered.AddDynamic(this, &ThisClass::HandleButtonUnhovered);
    JoinButton->OnUnhovered.AddDynamic(this, &ThisClass::HandleButtonUnhovered);
    SinglePlayerButton->OnUnhovered.AddDynamic(this, &ThisClass::HandleButtonUnhovered);
}

UButton* UCoopMenuWidget::BuildButton(const FString& Tag, const FString& Label)
{
    UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), *(Tag + TEXT("Button")));
    UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), *(Tag + TEXT("Label")));
    Text->SetText(FText::FromString(Label));
    Text->SetJustification(ETextJustify::Center);
    Text->SetFont(UGlobalGameData::Get(this)->MenuButtonFont);
    Text->SetColorAndOpacity(FSlateColor(FLinearColor(0.92f, 0.93f, 0.95f, 1.0f)));
    Text->SetShadowOffset(FVector2D(1.0f, 2.0f));
    Text->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.85f));

    FButtonStyle Style = Button->GetStyle();
    const auto MakeBrush = [](const FLinearColor& Color, float OutlineAlpha)
    {
        FSlateBrush Brush;
        Brush.DrawAs = ESlateBrushDrawType::Box;
        Brush.TintColor = FSlateColor(Color);
        Brush.OutlineSettings.Width = 1.5f;
        Brush.OutlineSettings.Color = FSlateColor(FLinearColor(0.85f, 0.08f, 0.035f, OutlineAlpha));
        return Brush;
    };
    Style.SetNormal(MakeBrush(FLinearColor(0.035f, 0.04f, 0.055f, 0.98f), 0.65f));
    Style.SetHovered(MakeBrush(FLinearColor(0.16f, 0.025f, 0.02f, 1.0f), 1.0f));
    Style.SetPressed(MakeBrush(FLinearColor(0.55f, 0.035f, 0.018f, 1.0f), 1.0f));
    Style.SetDisabled(MakeBrush(FLinearColor(0.025f, 0.025f, 0.03f, 0.7f), 0.2f));
    Style.NormalPadding = FMargin(18.0f, 13.0f);
    Style.PressedPadding = FMargin(18.0f, 15.0f, 18.0f, 11.0f);
    Button->SetStyle(Style);
    Button->SetContent(Text);
    return Button;
}

void UCoopMenuWidget::NativeConstruct()
{
    Super::NativeConstruct();
    IntroElapsed = 0.0f;
    SetRenderOpacity(0.0f);
    if (UEOSSessionSubsystem* Sessions = GetGameInstance()->GetSubsystem<UEOSSessionSubsystem>())
    {
        Sessions->OnStatusChanged.AddUniqueDynamic(this, &ThisClass::HandleStatusChanged);
        ApplyLobbyMode();
        HandleStatusChanged(Sessions->GetLastStatus());
    }
}

void UCoopMenuWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    IntroElapsed += InDeltaTime;
    SetRenderOpacity(FMath::Clamp(IntroElapsed / 0.35f, 0.0f, 1.0f));

    if (TitleText)
    {
        const float Pulse = 1.0f + FMath::Sin(IntroElapsed * 2.3f) * 0.018f;
        TitleText->SetRenderScale(FVector2D(Pulse));
        TitleText->SetRenderTranslation(FVector2D(0.0f, FMath::Lerp(-28.0f, 0.0f,
            FMath::Clamp(IntroElapsed / 0.55f, 0.0f, 1.0f))));
    }

    for (int32 Index = 0; Index < MenuButtons.Num(); ++Index)
    {
        if (UButton* Button = MenuButtons[Index])
        {
            const float Reveal = FMath::Clamp((IntroElapsed - 0.12f * Index) / 0.42f, 0.0f, 1.0f);
            const float Ease = 1.0f - FMath::Square(1.0f - Reveal);
            Button->SetRenderOpacity(Reveal);
            Button->SetRenderTranslation(FVector2D((1.0f - Ease) * 54.0f, 0.0f));
            const float TargetScale = HoveredButtonIndex == Index ? 1.045f : 1.0f;
            const FVector2D CurrentScale = Button->GetRenderTransform().Scale;
            Button->SetRenderScale(FMath::Vector2DInterpTo(CurrentScale, FVector2D(TargetScale), InDeltaTime, 14.0f));
        }
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

void UCoopMenuWidget::HandleHostHovered() { HoveredButtonIndex = 0; }
void UCoopMenuWidget::HandleJoinHovered() { HoveredButtonIndex = 1; }
void UCoopMenuWidget::HandleSoloHovered() { HoveredButtonIndex = 2; }
void UCoopMenuWidget::HandleButtonUnhovered() { HoveredButtonIndex = INDEX_NONE; }

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
