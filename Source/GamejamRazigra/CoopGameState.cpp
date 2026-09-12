#include "CoopGameState.h"

#include "CoopPlayerController.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Net/UnrealNetwork.h"

void ACoopGameState::OnRep_SharedHero()
{
    if (!SharedHero)
    {
        return;
    }
    for (TActorIterator<ACoopPlayerController> It(GetWorld()); It; ++It)
    {
        if (It->IsLocalController())
        {
            It->BindToSharedHero(SharedHero);
        }
    }
}

void ACoopGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ACoopGameState, SharedHero);
    DOREPLIFETIME(ACoopGameState, ConnectedPlayerCount);
}
