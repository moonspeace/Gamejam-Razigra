#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "SharedHeroCharacter.h"
#include "CoopPlayerController.generated.h"

class ASharedHeroCharacter;
class AHologramPuzzle;
enum class EPuzzleDirection : uint8;
class UCoopHudWidget;

/** Owns one network player's input, while both players view the same pawn. */
UCLASS(Blueprintable)
class GAMEJAMRAZIGRA_API ACoopPlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    ACoopPlayerController();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void PlayerTick(float DeltaTime) override;
    virtual void SetupInputComponent() override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    void SetPlayerSlot(int32 NewSlot);

    UFUNCTION(BlueprintCallable, Category="Razigra")
    void BindToSharedHero(ASharedHeroCharacter* Hero);

    /** Local-only HUD and camera response to replicated hero damage. */
    void HandleHeroDamaged(float DamageAmount);

    /** Local-only crosshair response to a replicated shot. */
    void HandleGunFired(bool bHit);

    /** Queues local media playback against the replicated server clock. */
    void StartLevelCinematic(double ServerStartTime);

    UFUNCTION(BlueprintCallable, Category="Zombie Zero|Game")
    void RequestRestartRun();

    /** Only the local controller that owns the listen server may reload a multiplayer run. */
    UFUNCTION(BlueprintPure, Category="Zombie Zero|Game")
    bool CanRestartRun() const;

    UFUNCTION(BlueprintPure, Category="Razigra")
    int32 GetPlayerSlot() const { return PlayerSlot; }

    UFUNCTION(BlueprintPure, Category="Razigra")
    int32 GetDisplayedRole() const;

    void SetPuzzleFocus(AHologramPuzzle* Puzzle);

protected:
    UFUNCTION(Server, Reliable)
    void ServerSetAction(EConsensusAction Action, bool bPressed);

    UFUNCTION(Server, Unreliable)
    void ServerSubmitLook(FVector2D LookDelta);

    UFUNCTION(Server, Reliable)
    void ServerSetRoleAbility(bool bPressed);

    UFUNCTION(Server, Reliable)
    void ServerSwitchSoloRole();

    UFUNCTION(Server, Reliable)
    void ServerRequestRestartRun();

    UFUNCTION(Server, Reliable)
    void ServerTogglePuzzleInteraction();

    UFUNCTION(Server, Reliable)
    void ServerPuzzleInput(EPuzzleDirection Direction, bool bActivate);

    UFUNCTION(Client, Reliable)
    void ClientSetPuzzleFocus(AHologramPuzzle* Puzzle);

    UFUNCTION(Client, Reliable)
    void ClientBindToSharedHero(ASharedHeroCharacter* Hero);

    UFUNCTION(Client, Reliable)
    void ClientStartIntroCinematic(double ServerStartTime);

    UFUNCTION()
    void OnRep_PlayerSlot();

private:
    friend class ACoopGameMode;

    UPROPERTY(ReplicatedUsing=OnRep_PlayerSlot)
    int32 PlayerSlot = INDEX_NONE;

    UPROPERTY(Replicated)
    int32 SoloAbilityRole = 0;

    UPROPERTY()
    TObjectPtr<ASharedHeroCharacter> SharedHero;

    UPROPERTY()
    TObjectPtr<UCoopHudWidget> GameplayHud;

    UPROPERTY()
    TObjectPtr<AHologramPuzzle> ActivePuzzle;

    FVector2D PendingLookInput = FVector2D::ZeroVector;
    float SharedHeroSearchTime = 0.0f;
    double PendingCinematicStartTime = -1.0;
    double LastCinematicStartTime = -1.0;

    void HoldScreenBlack();
    void FadeScreenIn();
    void ShowGameplayHud();
    void HideGameplayHud();
    void SetAction(EConsensusAction Action, bool bPressed);
    void MoveForwardPressed();
    void MoveForwardReleased();
    void MoveBackwardPressed();
    void MoveBackwardReleased();
    void MoveLeftPressed();
    void MoveLeftReleased();
    void MoveRightPressed();
    void MoveRightReleased();
    void JumpPressed();
    void JumpReleased();
    void CrouchPressed();
    void CrouchReleased();
    void FirePressed();
    void FireReleased();
    void AbilityPressed();
    void AbilityReleased();
    void SwitchSoloRole();
    void InteractPressed();
    void LookX(float Value);
    void LookY(float Value);
};
