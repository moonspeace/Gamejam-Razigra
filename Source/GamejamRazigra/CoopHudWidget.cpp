#include "CoopHudWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "CoopGameState.h"
#include "CoopPlayerController.h"
#include "Engine/World.h"
#include "GlobalGameData.h"

namespace RazigraHud
{
    static const FLinearColor ActiveTextColor(1.0f, 1.0f, 1.0f, 1.0f);
    static const FLinearColor MutedTextColor(0.58f, 0.62f, 0.68f, 1.0f);
    static const FLinearColor MeterBackgroundColor(0.04f, 0.05f, 0.07f, 0.55f);
    static constexpr float KeyWidth = 52.0f;
    static constexpr float ActionKeyWidth = 78.0f;
    static constexpr float SpaceKeyWidth = 112.0f;
    static constexpr float KeyHeight = 44.0f;
    static constexpr float KeyGap = 3.0f;
    static constexpr float MeterThickness = 10.0f;
    static constexpr float MeterLength = 96.0f;
}

UCoopHudWidget::UCoopHudWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
    , PlayerOneColor(0.85f, 0.13f, 0.16f, 0.95f)
    , PlayerTwoColor(0.09f, 0.45f, 0.95f, 0.95f)
    , ConsensusColor(0.10f, 0.76f, 0.35f, 0.98f)
    , IdleColor(0.05f, 0.06f, 0.08f, 0.78f)
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

    // No panel behind the keys: the key caps are the only chrome.
    UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("HudRow"));
    if (UOverlaySlot* RowSlot = Root->AddChildToOverlay(Row))
    {
        RowSlot->SetHorizontalAlignment(HAlign_Center);
        RowSlot->SetVerticalAlignment(VAlign_Bottom);
        RowSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 34.0f));
    }

    // Who am I, first thing on the left.
    if (UHorizontalBoxSlot* LegendSlot = Row->AddChildToHorizontalBox(BuildLegend()))
    {
        LegendSlot->SetVerticalAlignment(VAlign_Center);
        LegendSlot->SetPadding(FMargin(0.0f, 0.0f, 22.0f, 0.0f));
    }

    // Two rows of key caps, positioned the way they sit on a keyboard:
    //     [W]
    //  [A][S][D]     [CTRL][SPACE][LMB]
    UVerticalBox* Keyboard = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Keyboard"));
    if (UHorizontalBoxSlot* KeyboardSlot = Row->AddChildToHorizontalBox(Keyboard))
    {
        KeyboardSlot->SetVerticalAlignment(VAlign_Center);
        KeyboardSlot->SetPadding(FMargin(0.0f, 0.0f, 22.0f, 0.0f));
    }

    UHorizontalBox* TopRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("TopRow"));
    TopRow->AddChildToHorizontalBox(BuildKeySpacer(TEXT("PadTopLeft")));
    TopRow->AddChildToHorizontalBox(BuildActionCard(EConsensusAction::MoveForward, TEXT("W"), RazigraHud::KeyWidth));
    TopRow->AddChildToHorizontalBox(BuildKeySpacer(TEXT("PadTopRight")));
    if (UVerticalBoxSlot* TopSlot = Keyboard->AddChildToVerticalBox(TopRow))
    {
        TopSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, RazigraHud::KeyGap * 2.0f));
    }

    UHorizontalBox* BottomRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("BottomRow"));
    BottomRow->AddChildToHorizontalBox(BuildActionCard(EConsensusAction::MoveLeft, TEXT("A"), RazigraHud::KeyWidth));
    BottomRow->AddChildToHorizontalBox(BuildActionCard(EConsensusAction::MoveBackward, TEXT("S"), RazigraHud::KeyWidth));
    BottomRow->AddChildToHorizontalBox(BuildActionCard(EConsensusAction::MoveRight, TEXT("D"), RazigraHud::KeyWidth));
    BottomRow->AddChildToHorizontalBox(BuildKeySpacer(TEXT("PadBottomGap")));
    BottomRow->AddChildToHorizontalBox(BuildActionCard(EConsensusAction::Crouch, TEXT("CTRL"), RazigraHud::ActionKeyWidth));
    BottomRow->AddChildToHorizontalBox(BuildActionCard(EConsensusAction::Jump, TEXT("SPACE"), RazigraHud::SpaceKeyWidth));
    BottomRow->AddChildToHorizontalBox(BuildActionCard(EConsensusAction::Fire, TEXT("LMB"), RazigraHud::ActionKeyWidth));
    Keyboard->AddChildToVerticalBox(BottomRow);

    // Aim is metered rather than lit: the bars replace the old mouse key cap.
    if (UHorizontalBoxSlot* MeterSlot = Row->AddChildToHorizontalBox(BuildAxisMeters()))
    {
        MeterSlot->SetVerticalAlignment(VAlign_Center);
    }
}

/** An invisible key-sized block, so the two rows line up like a real keyboard. */
UWidget* UCoopHudWidget::BuildKeySpacer(const FString& Tag)
{
    USizeBox* Spacer = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), *Tag);
    Spacer->SetWidthOverride(RazigraHud::KeyWidth + RazigraHud::KeyGap * 2.0f);
    Spacer->SetHeightOverride(RazigraHud::KeyHeight);
    return Spacer;
}

UWidget* UCoopHudWidget::BuildActionCard(EConsensusAction Action, const FString& KeyText, float Width)
{
    const FString Tag = FString::Printf(TEXT("Card_%d"), static_cast<int32>(Action));

    USizeBox* CardBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), *(Tag + TEXT("Box")));
    CardBox->SetWidthOverride(Width);
    CardBox->SetHeightOverride(RazigraHud::KeyHeight);

    UBorder* Card = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), *Tag);
    Card->SetBrushColor(IdleColor);
    Card->SetPadding(FMargin(2.0f));
    Card->SetHorizontalAlignment(HAlign_Center);
    Card->SetVerticalAlignment(VAlign_Center);
    CardBox->SetContent(Card);

    const UGlobalGameData* Data = UGlobalGameData::Get(this);
    UTextBlock* Key = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), *(Tag + TEXT("Key")));
    Key->SetText(FText::FromString(KeyText));
    Key->SetFont(KeyText.Len() > 1 ? Data->HudKeyLabelFont : Data->HudKeyFont);
    Key->SetJustification(ETextJustify::Center);
    Key->SetColorAndOpacity(FSlateColor(RazigraHud::MutedTextColor));
    Card->SetContent(Key);

    ActionCards.Add(Card);
    ActionKeyLabels.Add(Key);
    CardActions.Add(Action);
    CardColors.Add(IdleColor);

    UHorizontalBox* Wrapper = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), *(Tag + TEXT("Wrap")));
    if (UHorizontalBoxSlot* WrapperSlot = Wrapper->AddChildToHorizontalBox(CardBox))
    {
        WrapperSlot->SetPadding(FMargin(RazigraHud::KeyGap, 0.0f));
    }
    return Wrapper;
}

UProgressBar* UCoopHudWidget::BuildMeter(const FString& Tag, bool bVertical, const FLinearColor& Color)
{
    UProgressBar* Meter = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), *Tag);
    Meter->WidgetStyle.BackgroundImage.TintColor = FSlateColor(RazigraHud::MeterBackgroundColor);
    Meter->WidgetStyle.FillImage.TintColor = FSlateColor(FLinearColor::White);
    Meter->BarFillType = bVertical ? EProgressBarFillType::BottomToTop : EProgressBarFillType::LeftToRight;
    Meter->SetFillColorAndOpacity(Color);
    // Half full is the neutral resting point, so the fill edge reads as a needle.
    Meter->SetPercent(0.5f);
    AxisMeters.Add(Meter);
    return Meter;
}

/**
 * Four meters: a horizontal pair stacked (yaw, player one over player two) and a vertical pair
 * side by side (pitch). Both players' contributions are visible at once, which is what makes the
 * summed aim readable.
 */
UWidget* UCoopHudWidget::BuildAxisMeters()
{
    UHorizontalBox* Meters = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("AxisMeters"));

    UVerticalBox* Horizontals = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("YawMeters"));
    for (int32 Index = 0; Index < 2; ++Index)
    {
        USizeBox* Box = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(),
            *FString::Printf(TEXT("YawBox_%d"), Index));
        Box->SetWidthOverride(RazigraHud::MeterLength);
        Box->SetHeightOverride(RazigraHud::MeterThickness);
        Box->SetContent(BuildMeter(FString::Printf(TEXT("YawMeter_%d"), Index), false,
            Index == 0 ? PlayerOneColor : PlayerTwoColor));
        if (UVerticalBoxSlot* BoxSlot = Horizontals->AddChildToVerticalBox(Box))
        {
            BoxSlot->SetPadding(FMargin(0.0f, RazigraHud::KeyGap));
        }
    }
    if (UHorizontalBoxSlot* HorizontalSlot = Meters->AddChildToHorizontalBox(Horizontals))
    {
        HorizontalSlot->SetVerticalAlignment(VAlign_Center);
        HorizontalSlot->SetPadding(FMargin(0.0f, 0.0f, 10.0f, 0.0f));
    }

    for (int32 Index = 0; Index < 2; ++Index)
    {
        USizeBox* Box = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(),
            *FString::Printf(TEXT("PitchBox_%d"), Index));
        Box->SetWidthOverride(RazigraHud::MeterThickness);
        Box->SetHeightOverride(RazigraHud::KeyHeight * 2.0f);
        Box->SetContent(BuildMeter(FString::Printf(TEXT("PitchMeter_%d"), Index), true,
            Index == 0 ? PlayerOneColor : PlayerTwoColor));
        if (UHorizontalBoxSlot* BoxSlot = Meters->AddChildToHorizontalBox(Box))
        {
            BoxSlot->SetVerticalAlignment(VAlign_Center);
            BoxSlot->SetPadding(FMargin(RazigraHud::KeyGap, 0.0f));
        }
    }
    return Meters;
}

/** Two numbered swatches. The local player's is solid, the other is dimmed: no caption needed. */
UWidget* UCoopHudWidget::BuildLegend()
{
    UHorizontalBox* Legend = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Legend"));
    for (int32 Index = 0; Index < 2; ++Index)
    {
        const FString Tag = FString::Printf(TEXT("Legend_%d"), Index);

        USizeBox* ChipBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), *(Tag + TEXT("Box")));
        ChipBox->SetWidthOverride(24.0f);
        ChipBox->SetHeightOverride(24.0f);

        UBorder* Chip = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), *Tag);
        Chip->SetBrushColor(Index == 0 ? PlayerOneColor : PlayerTwoColor);
        Chip->SetHorizontalAlignment(HAlign_Center);
        Chip->SetVerticalAlignment(VAlign_Center);
        ChipBox->SetContent(Chip);

        UTextBlock* Digit = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), *(Tag + TEXT("Digit")));
        Digit->SetText(FText::AsNumber(Index + 1));
        Digit->SetFont(UGlobalGameData::Get(this)->HudKeyFont);
        Digit->SetJustification(ETextJustify::Center);
        Digit->SetColorAndOpacity(FSlateColor(RazigraHud::ActiveTextColor));
        Chip->SetContent(Digit);

        LegendChips.Add(Chip);
        if (UHorizontalBoxSlot* ChipSlot = Legend->AddChildToHorizontalBox(ChipBox))
        {
            ChipSlot->SetVerticalAlignment(VAlign_Center);
            ChipSlot->SetPadding(FMargin(RazigraHud::KeyGap, 0.0f));
        }
    }
    return Legend;
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

    for (int32 Index = 0; Index < LegendChips.Num(); ++Index)
    {
        if (UBorder* Chip = LegendChips[Index])
        {
            FLinearColor ChipColor = Index == 0 ? PlayerOneColor : PlayerTwoColor;
            if (LocalIndex != INDEX_NONE && LocalIndex != Index)
            {
                ChipColor.A *= 0.3f;
            }
            Chip->SetBrushColor(ChipColor);
        }
    }

    // Axis meters. 0.5 is centre; the fill edge swings either side of it with the mouse delta.
    const float MeterRange = FMath::Max(0.01f, UGlobalGameData::Get(this)->LookMeterRange);
    for (int32 Index = 0; Index < AxisMeters.Num(); ++Index)
    {
        UProgressBar* Meter = AxisMeters[Index];
        if (!Meter)
        {
            continue;
        }
        const int32 Participant = Index % 2;
        const bool bVertical = Index >= 2;
        const FVector2D Axis = Hero ? Hero->GetParticipantLookAxis(Participant) : FVector2D::ZeroVector;
        // Screen-space pitch is inverted relative to the raw axis, so up on the mouse reads as up.
        const float Value = bVertical ? -Axis.Y : Axis.X;
        const float Target = 0.5f + 0.5f * FMath::Clamp(Value / MeterRange, -1.0f, 1.0f);
        Meter->SetPercent(FMath::FInterpTo(Meter->GetPercent(), Target, InDeltaTime, ColorBlendSpeed));
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
        if (UTextBlock* Key = ActionKeyLabels[Index])
        {
            Key->SetColorAndOpacity(FSlateColor(bActive ? RazigraHud::ActiveTextColor : RazigraHud::MutedTextColor));
        }
    }
}
