#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PuzzleBoardWidget.generated.h"

class AHologramPuzzle;

/** Native renderer for the world-space holographic puzzle board. */
UCLASS()
class GAMEJAMRAZIGRA_API UPuzzleBoardWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    void SetPuzzle(AHologramPuzzle* InPuzzle) { Puzzle = InPuzzle; InvalidateLayoutAndVolatility(); }

    /** Extra glow passes drawn under every stroke. The actor drives this from BloomIntensity. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Puzzle|Style", meta=(ClampMin="0"))
    float GlowStrength = 1.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Puzzle|Style")
    FLinearColor StructureColor = FLinearColor(0.30f, 0.90f, 1.0f, 1.0f);

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Puzzle|Style")
    FLinearColor SelectionColor = FLinearColor(1.0f, 0.92f, 0.35f, 1.0f);

    /** Match the HUD: the bracket takes this colour while that player waits for the other. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Puzzle|Style")
    FLinearColor PlayerOneColor = FLinearColor(0.85f, 0.13f, 0.16f, 1.0f);

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Puzzle|Style")
    FLinearColor PlayerTwoColor = FLinearColor(0.09f, 0.45f, 0.95f, 1.0f);

protected:
    virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
        const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
        int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
private:
    UPROPERTY() TObjectPtr<AHologramPuzzle> Puzzle;

    void Stroke(FSlateWindowElementList& Elements, int32 Layer, const FGeometry& Geometry,
        const TArray<FVector2D>& Points, const FLinearColor& Color, float Thickness, bool bGlow) const;
    void Shape(FSlateWindowElementList& Elements, int32 Layer, const FGeometry& Geometry,
        const TArray<FVector2D>& Points, const FLinearColor& Color, float Thickness) const;
    void Hatch(FSlateWindowElementList& Elements, int32 Layer, const FGeometry& Geometry,
        const TArray<FVector2D>& Poly, const FLinearColor& Color, float Spacing) const;
    void Ring(FSlateWindowElementList& Elements, int32 Layer, const FGeometry& Geometry,
        const FVector2D& Centre, float Radius, const FLinearColor& Color, float Thickness) const;
};
