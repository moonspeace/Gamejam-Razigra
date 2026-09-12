#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SharedHeroCharacter.h"
#include "CoopHudWidget.generated.h"

class UBorder;
class UHorizontalBox;
class UTextBlock;

/**
 * In-game consensus readout. Every shared action is drawn as a key card that lights up
 * red for player one, blue for player two, and green once both players agree.
 * Built entirely in C++; Blueprint subclasses may replace the visuals.
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
    TArray<TObjectPtr<UTextBlock>> ActionNameLabels;

    UPROPERTY()
    TObjectPtr<UTextBlock> LocalPlayerLabel;

    TArray<EConsensusAction> CardActions;
    TArray<FLinearColor> CardColors;

    UWidget* BuildActionCard(EConsensusAction Action, const FString& KeyText, const FString& NameText);
    UWidget* BuildLegendEntry(const FString& Tag, const FString& Text, const FLinearColor& Color);
    ASharedHeroCharacter* ResolveSharedHero() const;
    int32 ResolveLocalParticipantIndex() const;
};
