#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SharedHeroCharacter.h"
#include "CoopHudWidget.generated.h"

class UBorder;
class UHorizontalBox;
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

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Zombie Zero|HUD Style")
    FLinearColor PanelColor;

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

    TArray<EConsensusAction> CardActions;
    TArray<FLinearColor> CardColors;

    UWidget* BuildActionCard(EConsensusAction Action, const FString& KeyText, float Width);
    UWidget* BuildMovementCluster();
    UWidget* BuildStackedCluster(const FString& Tag, UWidget* Top, UWidget* Bottom);
    UWidget* BuildLegend();
    ASharedHeroCharacter* ResolveSharedHero() const;
    int32 ResolveLocalParticipantIndex() const;
};
