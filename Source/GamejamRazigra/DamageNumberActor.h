#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DamageNumberActor.generated.h"

class APlayerController;
class USceneComponent;
class UTextBlock;
class UWidgetComponent;

/** Client-only world text spawned by a replicated zombie damage notification. */
UCLASS(NotBlueprintable, Transient)
class GAMEJAMRAZIGRA_API ADamageNumberActor : public AActor
{
    GENERATED_BODY()

public:
    ADamageNumberActor();

    virtual void Tick(float DeltaSeconds) override;

    void InitializeDamageNumber(float DamageAmount, APlayerController* LocalController);

private:
    UPROPERTY(VisibleAnywhere)
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UWidgetComponent> DamageWidget;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> DamageText;

    TWeakObjectPtr<APlayerController> ViewController;
    FLinearColor BaseColor = FLinearColor::White;
    float Lifetime = 1.0f;
    float RiseSpeed = 40.0f;
    float Elapsed = 0.0f;
};
