#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "SharedHeroCharacter.h"
#include "CoopPlayerController.generated.h"

class ASharedHeroCharacter;
class UCoopHudWidget;

/** Owns one network player's input, while both players view the same pawn. */
UCLASS(Blueprintable)
class GAMEJAMRAZIGRA_API ACoopPlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    ACoopPlayerController();

    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void PlayerTick(float DeltaTime) override;
    virtual void SetupInputComponent() override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    void SetPlayerSlot(int32 NewSlot);

    UFUNCTION(BlueprintCallable, Category="Razigra")
    void BindToSharedHero(ASharedHeroCharacter* Hero);

    UFUNCTION(BlueprintPure, Category="Razigra")
    int32 GetPlayerSlot() const { return PlayerSlot; }

protected:
    UFUNCTION(Server, Reliable)
    void ServerSetAction(EConsensusAction Action, bool bPressed);

    UFUNCTION(Server, Unreliable)
    void ServerSubmitLook(FVector2D LookDelta);

    UFUNCTION(Client, Reliable)
    void ClientBindToSharedHero(ASharedHeroCharacter* Hero);

    UFUNCTION()
    void OnRep_PlayerSlot();

private:
    friend class ACoopGameMode;

    UPROPERTY(ReplicatedUsing=OnRep_PlayerSlot)
    int32 PlayerSlot = INDEX_NONE;

    UPROPERTY()
    TObjectPtr<ASharedHeroCharacter> SharedHero;

    UPROPERTY()
    TObjectPtr<UCoopHudWidget> GameplayHud;

    FVector2D PendingLookInput = FVector2D::ZeroVector;
    float SharedHeroSearchTime = 0.0f;

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
    void LookX(float Value);
    void LookY(float Value);
};
