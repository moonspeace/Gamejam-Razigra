#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "CoopGameState.generated.h"

class ASharedHeroCharacter;

UCLASS(Blueprintable)
class GAMEJAMRAZIGRA_API ACoopGameState : public AGameStateBase
{
    GENERATED_BODY()

public:
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UPROPERTY(ReplicatedUsing=OnRep_SharedHero, BlueprintReadOnly, Category="Razigra")
    TObjectPtr<ASharedHeroCharacter> SharedHero;

    UPROPERTY(Replicated, BlueprintReadOnly, Category="Razigra")
    int32 ConnectedPlayerCount = 0;

    UFUNCTION()
    void OnRep_SharedHero();
};
