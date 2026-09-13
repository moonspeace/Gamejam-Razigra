#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HologramPuzzle.generated.h"

class ASharedHeroCharacter;
class UCameraComponent;
class USceneComponent;
class UWidgetComponent;
class UPuzzleBoardWidget;

UENUM(BlueprintType)
enum class EPuzzleCellType : uint8 { Empty, Start, End, Mirror, Blocker, ColorSwitch };

UENUM(BlueprintType)
enum class EPuzzleLaserColor : uint8 { Red, Green, Blue };

UENUM()
enum class EPuzzleDirection : uint8 { North, East, South, West };

USTRUCT(BlueprintType)
struct FPuzzleCell
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) EPuzzleCellType Type = EPuzzleCellType::Empty;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) uint8 Rotation = 0;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) EPuzzleLaserColor Color = EPuzzleLaserColor::Red;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) bool bActive = false;
};

USTRUCT()
struct FPuzzleLaserSegment
{
    GENERATED_BODY()
    FIntPoint From = FIntPoint::ZeroValue;
    FIntPoint To = FIntPoint::ZeroValue;
    EPuzzleLaserColor Color = EPuzzleLaserColor::Red;
};

/** Placeable, replicated Witness-style RGB laser puzzle projected as a hologram. */
UCLASS(Blueprintable)
class GAMEJAMRAZIGRA_API AHologramPuzzle : public AActor
{
    GENERATED_BODY()

public:
    AHologramPuzzle();
    virtual void BeginPlay() override;
    virtual void OnConstruction(const FTransform& Transform) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    /** 0 = easy, 1 = medium, 2 = hard. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Puzzle", meta=(ClampMin="0", ClampMax="2"))
    int32 PuzzleId = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Puzzle", meta=(ClampMin="100"))
    float InteractionDistance = 350.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Puzzle|Camera", meta=(ClampMin="0"))
    float ViewBlendTime = 0.55f;

    /**
     * Multiplies the whole projection. Keep it white unless you want to bias every colour:
     * a cyan tint here zeroes the red channel and turns the red beam black.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Puzzle|Hologram")
    FLinearColor HologramTint = FLinearColor::White;

    /**
     * Pushes the projection's emissive above 1 so the scene's bloom picks it up. The widget's
     * own render target is 8-bit, so this tint multiply is what actually gets it into HDR.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Puzzle|Hologram", meta=(ClampMin="1.0", ClampMax="12.0"))
    float BloomIntensity = 3.5f;

    UFUNCTION(BlueprintImplementableEvent, Category="Puzzle")
    void BP_OnPuzzleSolved();

    UFUNCTION(BlueprintPure, Category="Puzzle") bool IsFocused() const { return bFocused; }
    UFUNCTION(BlueprintPure, Category="Puzzle") bool IsSolved() const { return bSolved; }
    bool IsWithinInteractionRange(const ASharedHeroCharacter* Hero) const;
    void SetFocused(bool bNewFocused);
    void MoveSelection(EPuzzleDirection Direction);
    void ActivateSelection();

    int32 GetBoardWidth() const { return BoardWidth; }
    int32 GetBoardHeight() const { return BoardHeight; }
    int32 GetSelectedCell() const { return SelectedCell; }
    const TArray<FPuzzleCell>& GetCells() const { return Cells; }
    const TArray<FPuzzleLaserSegment>& GetLaserPath() const { return LaserPath; }

protected:
    UFUNCTION() void OnRep_State();
    UFUNCTION(NetMulticast, Reliable) void MulticastSolved();

private:
    UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> Root;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UWidgetComponent> Projection;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UCameraComponent> PuzzleCamera;
    UPROPERTY(Transient) TObjectPtr<UPuzzleBoardWidget> BoardWidget;

    UPROPERTY(ReplicatedUsing=OnRep_State) TArray<FPuzzleCell> Cells;
    UPROPERTY(ReplicatedUsing=OnRep_State) int32 SelectedCell = INDEX_NONE;
    UPROPERTY(ReplicatedUsing=OnRep_State) bool bFocused = false;
    UPROPERTY(ReplicatedUsing=OnRep_State) bool bSolved = false;

    int32 BoardWidth = 5;
    int32 BoardHeight = 5;
    TArray<FPuzzleLaserSegment> LaserPath;

    void BuildPuzzle();
    void ApplyProjectionTint();
    void RecomputeLaser();
    void RefreshWidget();
    bool TraceLaser(TArray<FPuzzleLaserSegment>& OutPath) const;
    int32 Index(int32 X, int32 Y) const { return Y * BoardWidth + X; }
    bool IsInteractive(int32 CellIndex) const;
};
