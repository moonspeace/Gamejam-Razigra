#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SharedHeroCharacter.h"
#include "CoopHudWidget.generated.h"

class UBorder;
class UButton;
class UHorizontalBox;
class UProgressBar;
class USizeBox;
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

    /** Called locally when the shared hero takes damage. */
    void ShowDamageFeedback(float DamageAmount);

    /** Gives every shot native feedback even when the hero Blueprint has no weapon effects. */
    void ShowFireFeedback(bool bHit);

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

    UPROPERTY()
    TObjectPtr<UProgressBar> HealthBar;

    UPROPERTY()
    TArray<TObjectPtr<UBorder>> DamageVignetteEdges;

    UPROPERTY()
    TObjectPtr<UBorder> CrosshairCircle;

    UPROPERTY()
    TObjectPtr<USizeBox> CrosshairBox;

    UPROPERTY()
    TObjectPtr<UTextBlock> KillText;

    UPROPERTY()
    TObjectPtr<UTextBlock> SmashText;

    UPROPERTY()
    TObjectPtr<UBorder> GameOverOverlay;

    TArray<EConsensusAction> CardActions;
    TArray<FLinearColor> CardColors;
    TArray<float> DamageVignetteWeights;
    TArray<FLinearColor> MeterBaseColors;
    float DamageFeedbackRemaining = 0.0f;
    float DamageFeedbackStrength = 0.0f;
    float FireFeedbackRemaining = 0.0f;
    bool bLastShotHit = false;
    bool bGameOverShown = false;
    int32 DisplayedKillCount = 0;
    float KillSmashRemaining = 0.0f;

    void BuildCombatIndicators(class UOverlay* Root);
    void BuildDamageVignette(class UOverlay* Root);
    UWidget* BuildActionCard(EConsensusAction Action, const FString& KeyText, float Width);
    UWidget* BuildKeySpacer(const FString& Tag);
    UWidget* BuildAxisMeters();
    UProgressBar* BuildMeter(const FString& Tag, bool bVertical, const FLinearColor& Color);
    UWidget* BuildLegend();
    void BuildScoreAndGameOver(class UOverlay* Root);
    UFUNCTION()
    void HandleRestartClicked();
    ASharedHeroCharacter* ResolveSharedHero() const;
    int32 ResolveLocalParticipantIndex() const;
};
