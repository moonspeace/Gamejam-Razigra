#include "CoopHudWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "CoopGameState.h"
#include "CoopPlayerController.h"
#include "Engine/World.h"
#include "Styling/CoreStyle.h"

namespace RazigraHud
{
    static const FLinearColor ActiveTextColor(1.0f, 1.0f, 1.0f, 1.0f);
    static const FLinearColor MutedTextColor(0.62f, 0.66f, 0.72f, 1.0f);
    static constexpr float CardWidth = 96.0f;
    static constexpr float CardHeight = 64.0f;
}

UCoopHudWidget::UCoopHudWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
    , PlayerOneColor(0.85f, 0.13f, 0.16f, 0.95f)
    , PlayerTwoColor(0.09f, 0.45f, 0.95f, 0.95f)
    , ConsensusColor(0.10f, 0.76f, 0.35f, 0.98f)
    , IdleColor(0.05f, 0.06f, 0.08f, 0.78f)
    , PanelColor(0.02f, 0.025f, 0.035f, 0.80f)
    , ColorBlendSpeed(14.0f)
{
}

void UCoopHudWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    if (WidgetTree->RootWidget)
    {
        return;
    }

    UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("HudRoot"));
    WidgetTree->RootWidget = Root;

    UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ConsensusPanel"));
    Panel->SetBrushColor(PanelColor);
    Panel->SetPadding(FMargin(20.0f, 14.0f, 20.0f, 16.0f));
    if (UOverlaySlot* PanelSlot = Root->AddChildToOverlay(Panel))
    {
        PanelSlot->SetHorizontalAlignment(HAlign_Center);
        PanelSlot->SetVerticalAlignment(VAlign_Bottom);
        PanelSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 34.0f));
    }

    UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("PanelColumn"));
    Panel->SetContent(Column);

    // Header: title on the left, colour legend on the right.
    UHorizontalBox* Header = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Header"));
    if (UVerticalBoxSlot* HeaderSlot = Column->AddChildToVerticalBox(Header))
    {
        HeaderSlot->SetPadding(FMargin(2.0f, 0.0f, 2.0f, 10.0f));
        HeaderSlot->SetHorizontalAlignment(HAlign_Fill);
    }

    UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("HudTitle"));
    Title->SetText(FText::FromString(TEXT("CONSENSUS")));
    Title->SetFont(FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), 15));
    Title->SetColorAndOpacity(FSlateColor(FLinearColor(0.85f, 0.16f, 0.10f)));
    if (UHorizontalBoxSlot* TitleSlot = Header->AddChildToHorizontalBox(Title))
    {
        TitleSlot->SetVerticalAlignment(VAlign_Center);
        TitleSlot->SetPadding(FMargin(0.0f, 0.0f, 18.0f, 0.0f));
    }

    LocalPlayerLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("LocalPlayerLabel"));
    LocalPlayerLabel->SetText(FText::FromString(TEXT("CONNECTING")));
    LocalPlayerLabel->SetFont(FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), 11));
    LocalPlayerLabel->SetColorAndOpacity(FSlateColor(RazigraHud::MutedTextColor));
    if (UHorizontalBoxSlot* LocalSlot = Header->AddChildToHorizontalBox(LocalPlayerLabel))
    {
        LocalSlot->SetVerticalAlignment(VAlign_Center);
        LocalSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    }

    Header->AddChildToHorizontalBox(BuildLegendEntry(TEXT("LegendOne"), TEXT("PLAYER 1"), PlayerOneColor));
    Header->AddChildToHorizontalBox(BuildLegendEntry(TEXT("LegendTwo"), TEXT("PLAYER 2"), PlayerTwoColor));
    Header->AddChildToHorizontalBox(BuildLegendEntry(TEXT("LegendBoth"), TEXT("BOTH"), ConsensusColor));

    // Action cards.
    UHorizontalBox* Cards = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ActionRow"));
    if (UVerticalBoxSlot* CardsSlot = Column->AddChildToVerticalBox(Cards))
    {
        CardsSlot->SetHorizontalAlignment(HAlign_Center);
    }

    Cards->AddChildToHorizontalBox(BuildActionCard(EConsensusAction::MoveForward, TEXT("W"), TEXT("FORWARD")));
    Cards->AddChildToHorizontalBox(BuildActionCard(EConsensusAction::MoveLeft, TEXT("A"), TEXT("LEFT")));
    Cards->AddChildToHorizontalBox(BuildActionCard(EConsensusAction::MoveBackward, TEXT("S"), TEXT("BACK")));
    Cards->AddChildToHorizontalBox(BuildActionCard(EConsensusAction::MoveRight, TEXT("D"), TEXT("RIGHT")));
    Cards->AddChildToHorizontalBox(BuildActionCard(EConsensusAction::Jump, TEXT("SPACE"), TEXT("JUMP")));
    Cards->AddChildToHorizontalBox(BuildActionCard(EConsensusAction::Crouch, TEXT("CTRL"), TEXT("CROUCH")));
    Cards->AddChildToHorizontalBox(BuildActionCard(EConsensusAction::Fire, TEXT("LMB"), TEXT("FIRE")));

    UTextBlock* Hint = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("HudHint"));
    Hint->SetText(FText::FromString(TEXT("The survivor only obeys an input while both players hold it.")));
    Hint->SetFont(FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 10));
    Hint->SetColorAndOpacity(FSlateColor(RazigraHud::MutedTextColor));
    Hint->SetJustification(ETextJustify::Center);
    if (UVerticalBoxSlot* HintSlot = Column->AddChildToVerticalBox(Hint))
    {
        HintSlot->SetPadding(FMargin(0.0f, 10.0f, 0.0f, 0.0f));
        HintSlot->SetHorizontalAlignment(HAlign_Center);
    }
}

UWidget* UCoopHudWidget::BuildLegendEntry(const FString& Tag, const FString& Text, const FLinearColor& Color)
{
    UHorizontalBox* Entry = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), *Tag);

    USizeBox* SwatchBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), *(Tag + TEXT("SwatchBox")));
    SwatchBox->SetWidthOverride(11.0f);
    SwatchBox->SetHeightOverride(11.0f);
    UBorder* Swatch = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), *(Tag + TEXT("Swatch")));
    Swatch->SetBrushColor(Color);
    SwatchBox->SetContent(Swatch);
    if (UHorizontalBoxSlot* SwatchSlot = Entry->AddChildToHorizontalBox(SwatchBox))
    {
        SwatchSlot->SetVerticalAlignment(VAlign_Center);
        SwatchSlot->SetPadding(FMargin(12.0f, 0.0f, 6.0f, 0.0f));
    }

    UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), *(Tag + TEXT("Label")));
    Label->SetText(FText::FromString(Text));
    Label->SetFont(FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 10));
    Label->SetColorAndOpacity(FSlateColor(RazigraHud::MutedTextColor));
    if (UHorizontalBoxSlot* LabelSlot = Entry->AddChildToHorizontalBox(Label))
    {
        LabelSlot->SetVerticalAlignment(VAlign_Center);
    }
    return Entry;
}

UWidget* UCoopHudWidget::BuildActionCard(EConsensusAction Action, const FString& KeyText, const FString& NameText)
{
    const FString Tag = FString::Printf(TEXT("Card_%d"), static_cast<int32>(Action));

    USizeBox* CardBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), *(Tag + TEXT("Box")));
    CardBox->SetWidthOverride(RazigraHud::CardWidth);
    CardBox->SetHeightOverride(RazigraHud::CardHeight);

    UBorder* Card = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), *Tag);
    Card->SetBrushColor(IdleColor);
    Card->SetPadding(FMargin(6.0f));
    Card->SetHorizontalAlignment(HAlign_Center);
    Card->SetVerticalAlignment(VAlign_Center);
    CardBox->SetContent(Card);

    UVerticalBox* CardColumn = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), *(Tag + TEXT("Column")));
    Card->SetContent(CardColumn);

    UTextBlock* Key = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), *(Tag + TEXT("Key")));
    Key->SetText(FText::FromString(KeyText));
    Key->SetFont(FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), 17));
    Key->SetJustification(ETextJustify::Center);
    Key->SetColorAndOpacity(FSlateColor(RazigraHud::MutedTextColor));
    if (UVerticalBoxSlot* KeySlot = CardColumn->AddChildToVerticalBox(Key))
    {
        KeySlot->SetHorizontalAlignment(HAlign_Center);
    }

    UTextBlock* Name = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), *(Tag + TEXT("Name")));
    Name->SetText(FText::FromString(NameText));
    Name->SetFont(FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 9));
    Name->SetJustification(ETextJustify::Center);
    Name->SetColorAndOpacity(FSlateColor(RazigraHud::MutedTextColor));
    if (UVerticalBoxSlot* NameSlot = CardColumn->AddChildToVerticalBox(Name))
    {
        NameSlot->SetHorizontalAlignment(HAlign_Center);
        NameSlot->SetPadding(FMargin(0.0f, 2.0f, 0.0f, 0.0f));
    }

    ActionCards.Add(Card);
    ActionKeyLabels.Add(Key);
    ActionNameLabels.Add(Name);
    CardActions.Add(Action);
    CardColors.Add(IdleColor);

    UHorizontalBox* Wrapper = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), *(Tag + TEXT("Wrapper")));
    if (UHorizontalBoxSlot* WrapperSlot = Wrapper->AddChildToHorizontalBox(CardBox))
    {
        WrapperSlot->SetPadding(FMargin(4.0f, 0.0f));
    }
    return Wrapper;
}

ASharedHeroCharacter* UCoopHudWidget::ResolveSharedHero() const
{
    if (const UWorld* World = GetWorld())
    {
        if (const ACoopGameState* State = World->GetGameState<ACoopGameState>())
        {
            return State->SharedHero;
        }
    }
    return nullptr;
}

int32 UCoopHudWidget::ResolveLocalParticipantIndex() const
{
    if (const ACoopPlayerController* Controller = Cast<ACoopPlayerController>(GetOwningPlayer()))
    {
        return Controller->GetPlayerSlot();
    }
    return INDEX_NONE;
}

void UCoopHudWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    const ASharedHeroCharacter* Hero = ResolveSharedHero();
    const int32 RequiredParticipants = Hero ? FMath::Max(1, Hero->GetRequiredConsensusParticipants()) : 2;
    const int32 LocalIndex = ResolveLocalParticipantIndex();

    if (LocalPlayerLabel)
    {
        if (LocalIndex == 0 || LocalIndex == 1)
        {
            LocalPlayerLabel->SetText(FText::FromString(FString::Printf(TEXT("YOU ARE PLAYER %d"), LocalIndex + 1)));
            LocalPlayerLabel->SetColorAndOpacity(FSlateColor(LocalIndex == 0 ? PlayerOneColor : PlayerTwoColor));
        }
        else
        {
            LocalPlayerLabel->SetText(FText::FromString(TEXT("CONNECTING")));
            LocalPlayerLabel->SetColorAndOpacity(FSlateColor(RazigraHud::MutedTextColor));
        }
    }

    for (int32 Index = 0; Index < ActionCards.Num(); ++Index)
    {
        const EConsensusAction Action = CardActions[Index];
        const bool bPlayerOne = Hero && Hero->IsActionPressedBy(0, Action);
        const bool bPlayerTwo = Hero && Hero->IsActionPressedBy(1, Action);

        FLinearColor Target = IdleColor;
        bool bActive = false;
        if (RequiredParticipants <= 1)
        {
            bActive = bPlayerOne || bPlayerTwo;
            Target = bActive ? ConsensusColor : IdleColor;
        }
        else if (bPlayerOne && bPlayerTwo)
        {
            Target = ConsensusColor;
            bActive = true;
        }
        else if (bPlayerOne)
        {
            Target = PlayerOneColor;
            bActive = true;
        }
        else if (bPlayerTwo)
        {
            Target = PlayerTwoColor;
            bActive = true;
        }

        CardColors[Index] = FMath::CInterpTo(CardColors[Index], Target, InDeltaTime, ColorBlendSpeed);
        if (UBorder* Card = ActionCards[Index])
        {
            Card->SetBrushColor(CardColors[Index]);
        }

        const FSlateColor TextColor(bActive ? RazigraHud::ActiveTextColor : RazigraHud::MutedTextColor);
        if (UTextBlock* Key = ActionKeyLabels[Index])
        {
            Key->SetColorAndOpacity(TextColor);
        }
        if (UTextBlock* Name = ActionNameLabels[Index])
        {
            Name->SetColorAndOpacity(TextColor);
        }
    }
}
