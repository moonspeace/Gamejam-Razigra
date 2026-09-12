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

    UPROPERTY(ReplicatedUsing=OnRep_LobbyPopulation, BlueprintReadOnly, Category="Razigra")
    int32 ConnectedPlayerCount = 0;

    UPROPERTY(ReplicatedUsing=OnRep_LobbyPopulation, BlueprintReadOnly, Category="Razigra")
    int32 RequiredPlayerCount = 2;

    UFUNCTION()
    void OnRep_SharedHero();

    /** Keeps a joining client's front end in step with the host's lobby. */
    UFUNCTION()
    void OnRep_LobbyPopulation();
};
