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
    FSlateFontInfo TitleFont;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Zombie Zero|Menu Style")
    FSlateFontInfo SubtitleFont;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Zombie Zero|Menu Style")
    FSlateFontInfo ButtonFont;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Zombie Zero|Menu Style")
    FSlateFontInfo StatusFont;

protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

private:
    UPROPERTY()
    TObjectPtr<UTextBlock> StatusText;

    UPROPERTY()
    TObjectPtr<UButton> HostButton;

    UPROPERTY()
    TObjectPtr<UButton> JoinButton;

    UPROPERTY()
    TObjectPtr<UButton> SinglePlayerButton;

    UFUNCTION()
    void HandleHostClicked();

    UFUNCTION()
    void HandleJoinClicked();

    UFUNCTION()
    void HandleSinglePlayerClicked();

    UFUNCTION()
    void HandleStatusChanged(const FString& NewStatus);
};
