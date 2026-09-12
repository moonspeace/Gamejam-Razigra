#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CoopMenuWidget.generated.h"

class UButton;
class UTextBlock;

/** Minimal no-Blueprint-required front end; Blueprint subclasses may replace its visuals. */
UCLASS(Blueprintable)
class GAMEJAMRAZIGRA_API UCoopMenuWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UCoopMenuWidget(const FObjectInitializer& ObjectInitializer);

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Zombie Zero|Menu Style")
    FLinearColor BackdropColor;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Zombie Zero|Menu Style")
    FLinearColor TitleColor;

protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
    UPROPERTY()
    TObjectPtr<UTextBlock> StatusText;

    UPROPERTY()
    TObjectPtr<UButton> HostButton;

    UPROPERTY()
    TObjectPtr<UButton> JoinButton;

    UPROPERTY()
    TObjectPtr<UButton> SinglePlayerButton;

    UPROPERTY()
    TObjectPtr<UTextBlock> TitleText;

    TArray<TObjectPtr<UButton>> MenuButtons;
    float IntroElapsed = 0.0f;
    int32 HoveredButtonIndex = INDEX_NONE;

    UButton* BuildButton(const FString& Tag, const FString& Label);

    /** Hides the launch buttons while the session is waiting for the other player. */
    void ApplyLobbyMode();

    UFUNCTION()
    void HandleHostClicked();

    UFUNCTION()
    void HandleJoinClicked();

    UFUNCTION()
    void HandleSinglePlayerClicked();

    UFUNCTION()
    void HandleHostHovered();
    UFUNCTION()
    void HandleJoinHovered();
    UFUNCTION()
    void HandleSoloHovered();
    UFUNCTION()
    void HandleButtonUnhovered();

    UFUNCTION()
    void HandleStatusChanged(const FString& NewStatus);
};
