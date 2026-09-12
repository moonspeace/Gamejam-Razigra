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
    static const FLinearColor MutedTextColor(0.58f, 0.62f, 0.68f, 1.0f);
    static constexpr float KeyWidth = 52.0f;
    static constexpr float WideKeyWidth = 116.0f;
    static constexpr float MouseKeyWidth = 74.0f;
    static constexpr float KeyHeight = 44.0f;
    static constexpr float KeyGap = 3.0f;
}

UCoopHudWidget::UCoopHudWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
    , PlayerOneColor(0.85f, 0.13f, 0.16f, 0.95f)
    , PlayerTwoColor(0.09f, 0.45f, 0.95f, 0.95f)
    , ConsensusColor(0.10f, 0.76f, 0.35f, 0.98f)
    , IdleColor(0.05f, 0.06f, 0.08f, 0.78f)
    , PanelColor(0.02f, 0.025f, 0.035f, 0.78f)
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
    Panel->SetPadding(FMargin(16.0f, 12.0f));
    if (UOverlaySlot* PanelSlot = Root->AddChildToOverlay(Panel))
    {
        PanelSlot->SetHorizontalAlignment(HAlign_Center);
        PanelSlot->SetVerticalAlignment(VAlign_Bottom);
        PanelSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 32.0f));
    }

    UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("HudRow"));
    Panel->SetContent(Row);

    // Movement keys sit where they sit on the keyboard: W above, A S D beneath it.
    if (UHorizontalBoxSlot* MoveSlot = Row->AddChildToHorizontalBox(BuildMovementCluster()))
    {
        MoveSlot->SetVerticalAlignment(VAlign_Center);
        MoveSlot->SetPadding(FMargin(0.0f, 0.0f, 18.0f, 0.0f));
    }

    // Space over Ctrl, matching the bottom row of the keyboard.
    UWidget* SpaceKey = BuildActionCard(EConsensusAction::Jump, TEXT("SPACE"), RazigraHud::WideKeyWidth);
    UWidget* CrouchKey = BuildActionCard(EConsensusAction::Crouch, TEXT("CTRL"), RazigraHud::WideKeyWidth);
    if (UHorizontalBoxSlot* KeysSlot = Row->AddChildToHorizontalBox(
        BuildStackedCluster(TEXT("BottomRowKeys"), SpaceKey, CrouchKey)))
    {
        KeysSlot->SetVerticalAlignment(VAlign_Center);
        KeysSlot->SetPadding(FMargin(0.0f, 0.0f, 18.0f, 0.0f));
    }

    // The mouse: click above, aim beneath.
    UWidget* FireKey = BuildActionCard(EConsensusAction::Fire, TEXT("LMB"), RazigraHud::MouseKeyWidth);
    UWidget* LookKey = BuildActionCard(EConsensusAction::Look, TEXT("MOUSE"), RazigraHud::MouseKeyWidth);
    if (UHorizontalBoxSlot* MouseSlot = Row->AddChildToHorizontalBox(
        BuildStackedCluster(TEXT("MouseKeys"), FireKey, LookKey)))
    {
        MouseSlot->SetVerticalAlignment(VAlign_Center);
        MouseSlot->SetPadding(FMargin(0.0f, 0.0f, 18.0f, 0.0f));
    }

    if (UHorizontalBoxSlot* LegendSlot = Row->AddChildToHorizontalBox(BuildLegend()))
    {
        LegendSlot->SetVerticalAlignment(VAlign_Center);
    }
}

UWidget* UCoopHudWidget::BuildMovementCluster()
{
    UVerticalBox* Cluster = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MoveCluster"));

    UHorizontalBox* TopRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("MoveTopRow"));
    TopRow->AddChildToHorizontalBox(BuildActionCard(EConsensusAction::MoveForward, TEXT("W"), RazigraHud::KeyWidth));
    if (UVerticalBoxSlot* TopSlot = Cluster->AddChildToVerticalBox(TopRow))
    {
        TopSlot->SetHorizontalAlignment(HAlign_Center);
        TopSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, RazigraHud::KeyGap * 2.0f));
    }

    UHorizontalBox* BottomRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("MoveBottomRow"));
    BottomRow->AddChildToHorizontalBox(BuildActionCard(EConsensusAction::MoveLeft, TEXT("A"), RazigraHud::KeyWidth));
    BottomRow->AddChildToHorizontalBox(BuildActionCard(EConsensusAction::MoveBackward, TEXT("S"), RazigraHud::KeyWidth));
    BottomRow->AddChildToHorizontalBox(BuildActionCard(EConsensusAction::MoveRight, TEXT("D"), RazigraHud::KeyWidth));
    if (UVerticalBoxSlot* BottomSlot = Cluster->AddChildToVerticalBox(BottomRow))
    {
        BottomSlot->SetHorizontalAlignment(HAlign_Center);
    }
    return Cluster;
}

UWidget* UCoopHudWidget::BuildStackedCluster(const FString& Tag, UWidget* Top, UWidget* Bottom)
{
    UVerticalBox* Cluster = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), *Tag);
    if (UVerticalBoxSlot* TopSlot = Cluster->AddChildToVerticalBox(Top))
    {
        TopSlot->SetHorizontalAlignment(HAlign_Center);
        TopSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, RazigraHud::KeyGap * 2.0f));
    }
    if (UVerticalBoxSlot* BottomSlot = Cluster->AddChildToVerticalBox(Bottom))
    {
        BottomSlot->SetHorizontalAlignment(HAlign_Center);
    }
    return Cluster;
}

UWidget* UCoopHudWidget::BuildActionCard(EConsensusAction Action, const FString& KeyText, float Width)
{
    const FString Tag = FString::Printf(TEXT("Card_%d"), static_cast<int32>(Action));

    USizeBox* CardBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), *(Tag + TEXT("Box")));
    CardBox->SetWidthOverride(Width);
    CardBox->SetHeightOverride(RazigraHud::KeyHeight);

    UBorder* Card = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), *Tag);
    Card->SetBrushColor(IdleColor);
    Card->SetPadding(FMargin(RazigraHud::KeyGap));
    Card->SetHorizontalAlignment(HAlign_Center);
    Card->SetVerticalAlignment(VAlign_Center);
    CardBox->SetContent(Card);

    UTextBlock* Key = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), *(Tag + TEXT("Key")));
    Key->SetText(FText::FromString(KeyText));
    Key->SetFont(FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), KeyText.Len() > 1 ? 12 : 19));
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
        Digit->SetFont(FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), 12));
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
