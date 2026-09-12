#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SharedHeroCharacter.h"
#include "CoopHudWidget.generated.h"

class UBorder;
class UHorizontalBox;
class UProgressBar;
class UTextBlock;
class UVerticalBox;

/**
 * In-game consensus readout. Every shared input is a key cap laid out the way it sits on a
 * keyboard, lit red for player one, blue for player two and green once both agree. Deliberately
 * wordless: colour and key letters carry the whole message.
 */
UCLASS(Blueprintable)
class GAMEJAMRAZIGRA_API UCoopHudWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UCoopHudWidget(const FObjectInitializer& ObjectInitializer);

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Zombie Zero|HUD Style")
    FLinearColor PlayerOneColor;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Zombie Zero|HUD Style")
    FLinearColor PlayerTwoColor;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Zombie Zero|HUD Style")
    FLinearColor ConsensusColor;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Zombie Zero|HUD Style")
    FLinearColor IdleColor;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Zombie Zero|HUD Style", meta=(ClampMin="0.1"))
    float ColorBlendSpeed;

protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
    UPROPERTY()
    TArray<TObjectPtr<UBorder>> ActionCards;

    UPROPERTY()
    TArray<TObjectPtr<UTextBlock>> ActionKeyLabels;

    UPROPERTY()
    TArray<TObjectPtr<UBorder>> LegendChips;

    /** Index 0/1 = player one/two horizontal (yaw) meters, then the vertical (pitch) meters. */
    UPROPERTY()
    TArray<TObjectPtr<UProgressBar>> AxisMeters;

    TArray<EConsensusAction> CardActions;
    TArray<FLinearColor> CardColors;

    UWidget* BuildActionCard(EConsensusAction Action, const FString& KeyText);
    UWidget* BuildKeySpacer(const FString& Tag);
    UWidget* BuildAxisMeters();
    UProgressBar* BuildMeter(const FString& Tag, bool bVertical, const FLinearColor& Color);
    UWidget* BuildLegend();
    ASharedHeroCharacter* ResolveSharedHero() const;
    int32 ResolveLocalParticipantIndex() const;
};
