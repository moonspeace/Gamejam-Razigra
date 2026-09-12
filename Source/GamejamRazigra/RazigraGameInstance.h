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

    UFUNCTION(BlueprintPure, Category="Razigra|Menu")
    bool IsMainMenuVisible() const;

private:
    UPROPERTY(Transient)
    TObjectPtr<UCoopMenuWidget> MainMenuWidget;

    FTimerHandle ShowMenuRetryTimer;
};
