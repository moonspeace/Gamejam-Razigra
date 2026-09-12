#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "RazigraGameInstance.generated.h"

class UCoopMenuWidget;

/** Owns the front end so it appears at Play startup independently of the loaded map or GameMode. */
UCLASS(Blueprintable)
class GAMEJAMRAZIGRA_API URazigraGameInstance : public UGameInstance
{
    GENERATED_BODY()

public:
    virtual void OnStart() override;

    UFUNCTION(BlueprintCallable, Category="Razigra|Menu")
    void ShowMainMenu();

    UFUNCTION(BlueprintCallable, Category="Razigra|Menu")
    void HideMainMenu();

    /**
     * Rebuilds or tears down the front end for the world that just loaded. Travel destroys
     * the owning player controller, so the widget has to be recreated against the new one.
     */
    UFUNCTION(BlueprintCallable, Category="Razigra|Menu")
    void RefreshFrontEnd();

    UFUNCTION(BlueprintPure, Category="Razigra|Menu")
    bool IsMainMenuVisible() const;

private:
    UPROPERTY(Transient)
    TObjectPtr<UCoopMenuWidget> MainMenuWidget;

    FTimerHandle ShowMenuRetryTimer;

    bool ShouldFrontEndBeVisible() const;
};
