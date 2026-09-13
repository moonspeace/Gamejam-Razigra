#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SharedHeroCharacter.h"
#include "CoopHudWidget.generated.h"

class UBorder;
class UButton;
class UHorizontalBox;
class UImage;
class UMediaPlayer;
class UMediaSoundComponent;
class UMediaTexture;
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

    void PlayIntroCinematic(double ServerStartTime);
    void PlayIntroCinematicNow();

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
    virtual void NativeDestruct() override;

private:
    UPROPERTY()
    TArray<TObjectPtr<UBorder>> ActionCards;

    UPROPERTY()
    TArray<TObjectPtr<UTextBlock>> ActionKeyLabels;

    UPROPERTY()
    TArray<TObjectPtr<UBorder>> LegendChips;

    UPROPERTY()
    TArray<TObjectPtr<UBorder>> AbilityCards;

    /** Index 0/1 = player one/two horizontal (yaw) meters, then the vertical (pitch) meters. */
    UPROPERTY()
    TArray<TObjectPtr<UProgressBar>> AxisMeters;

    UPROPERTY()
    TObjectPtr<UProgressBar> HealthBar;

    UPROPERTY()
    TObjectPtr<UProgressBar> RecoilBar;

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

    UPROPERTY()
    TObjectPtr<UButton> RestartRunButton;

    UPROPERTY()
    TObjectPtr<UTextBlock> RestartRunLabel;

    UPROPERTY()
    TObjectPtr<UBorder> CinematicOverlay;

    UPROPERTY()
    TObjectPtr<UImage> CinematicImage;

    UPROPERTY()
    TObjectPtr<UMediaPlayer> CinematicPlayer;

    UPROPERTY()
    TObjectPtr<UMediaTexture> CinematicTexture;

    UPROPERTY()
    TObjectPtr<UMediaSoundComponent> CinematicSound;

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
    float OverheatFlashTime = 0.0f;
    double PendingCinematicServerTime = -1.0;
    bool bCinematicMediaReady = false;
    bool bStartCinematicImmediately = false;

    void BuildCombatIndicators(class UOverlay* Root);
    void BuildDamageVignette(class UOverlay* Root);
    UWidget* BuildActionCard(EConsensusAction Action, const FString& KeyText, float Width);
    UWidget* BuildKeySpacer(const FString& Tag);
    UWidget* BuildAxisMeters();
    UProgressBar* BuildMeter(const FString& Tag, bool bVertical, const FLinearColor& Color);
    UWidget* BuildLegend();
    UWidget* BuildRoleAbilityCard();
    void BuildScoreAndGameOver(class UOverlay* Root);
    void BuildCinematicOverlay(class UOverlay* Root);
    UFUNCTION() void HandleCinematicOpened(FString OpenedUrl);
    UFUNCTION() void HandleCinematicEnded();
    UFUNCTION()
    void HandleRestartClicked();
    ASharedHeroCharacter* ResolveSharedHero() const;
    int32 ResolveLocalParticipantIndex() const;
};
