#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "CoopGameMode.generated.h"

class ACoopPlayerController;
class ASharedHeroCharacter;
class USceneComponent;

UCLASS(Blueprintable)
class GAMEJAMRAZIGRA_API ACoopGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    ACoopGameMode();

    virtual void StartPlay() override;
    virtual void PostLogin(APlayerController* NewPlayer) override;
    virtual void HandleSeamlessTravelPlayer(AController*& Controller) override;
    virtual void Logout(AController* Exiting) override;
    virtual void PreLogin(const FString& Options, const FString& Address,
        const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;

    /** Server-side reset that preserves the level and connected session. */
    void RestartRunInPlace();

protected:
    UFUNCTION(BlueprintImplementableEvent, Category="Razigra", meta=(DisplayName="On Shared Hero Spawned"))
    void BP_OnSharedHeroSpawned(ASharedHeroCharacter* Hero);

private:
    TMap<TWeakObjectPtr<ACoopPlayerController>, int32> AssignedSlots;
    TMap<TWeakObjectPtr<AActor>, FTransform> InitialGateTransforms;
    TMap<TWeakObjectPtr<USceneComponent>, FTransform> InitialGateComponentTransforms;

    bool CanStartGameplay() const;
    int32 GetRequiredPlayers() const;
    void RegisterPlayer(ACoopPlayerController* CoopController);
    void EnsureSharedHero();
    int32 AllocateSlot() const;
    void RefreshConnectedCount();
    void CaptureInitialGateState();
    void ResetGates();
};
