#include "PuzzleBoardWidget.h"

#include "HologramPuzzle.h"
#include "Rendering/DrawElements.h"

namespace
{
    FLinearColor BeamColor(EPuzzleLaserColor Color)
    {
        switch (Color)
        {
        case EPuzzleLaserColor::Green: return FLinearColor(0.05f, 1.0f, 0.15f, 1.0f);
        case EPuzzleLaserColor::Blue:  return FLinearColor(0.15f, 0.45f, 1.0f, 1.0f);
        default:                       return FLinearColor(1.0f, 0.08f, 0.06f, 1.0f);
        }
    }
}

void UPuzzleBoardWidget::Stroke(FSlateWindowElementList& Elements, int32 Layer, const FGeometry& Geometry,
    const TArray<FVector2D>& Points, const FLinearColor& Color, float Thickness, bool bGlow) const
{
    if (Points.Num() < 2) return;

    if (bGlow && GlowStrength > 0.0f)
    {
        // Two soft passes under the stroke. The scene bloom does the rest, but this keeps the
        // shapes glowing even where the post-process threshold would not catch them.
        FLinearColor Wide = Color;  Wide.A = Color.A * 0.10f * GlowStrength;
        FLinearColor Mid = Color;   Mid.A  = Color.A * 0.22f * GlowStrength;
        FSlateDrawElement::MakeLines(Elements, Layer, Geometry.ToPaintGeometry(), Points,
            ESlateDrawEffect::None, Wide, true, Thickness * 6.0f);
        FSlateDrawElement::MakeLines(Elements, Layer, Geometry.ToPaintGeometry(), Points,
            ESlateDrawEffect::None, Mid, true, Thickness * 3.0f);
    }
    FSlateDrawElement::MakeLines(Elements, Layer, Geometry.ToPaintGeometry(), Points,
        ESlateDrawEffect::None, Color, true, Thickness);
}

void UPuzzleBoardWidget::Shape(FSlateWindowElementList& Elements, int32 Layer, const FGeometry& Geometry,
    const TArray<FVector2D>& Points, const FLinearColor& Color, float Thickness) const
{
    TArray<FVector2D> Closed = Points;
    Closed.Add(Points[0]);
    Stroke(Elements, Layer, Geometry, Closed, Color, Thickness, true);
}

/** Scanline fill, so a solid piece still reads as a projection rather than flat paint. */
void UPuzzleBoardWidget::Hatch(FSlateWindowElementList& Elements, int32 Layer, const FGeometry& Geometry,
    const TArray<FVector2D>& Poly, const FLinearColor& Color, float Spacing) const
{
    float MinY = FLT_MAX, MaxY = -FLT_MAX;
    for (const FVector2D& P : Poly) { MinY = FMath::Min(MinY, P.Y); MaxY = FMath::Max(MaxY, P.Y); }

    for (float Y = MinY + Spacing * 0.5f; Y < MaxY; Y += Spacing)
    {
        float Left = FLT_MAX, Right = -FLT_MAX;
        for (int32 I = 0; I < Poly.Num(); ++I)
        {
            const FVector2D& P = Poly[I];
            const FVector2D& Q = Poly[(I + 1) % Poly.Num()];
            if ((P.Y <= Y && Q.Y > Y) || (Q.Y <= Y && P.Y > Y))
            {
                const float X = P.X + (Y - P.Y) / (Q.Y - P.Y) * (Q.X - P.X);
                Left = FMath::Min(Left, X);
                Right = FMath::Max(Right, X);
            }
        }
        if (Right > Left)
        {
            Stroke(Elements, Layer, Geometry, {FVector2D(Left, Y), FVector2D(Right, Y)}, Color, 1.0f, false);
        }
    }
}

void UPuzzleBoardWidget::Ring(FSlateWindowElementList& Elements, int32 Layer, const FGeometry& Geometry,
    const FVector2D& Centre, float Radius, const FLinearColor& Color, float Thickness) const
{
    TArray<FVector2D> Points;
    for (int32 Step = 0; Step <= 32; ++Step)
    {
        const float Angle = 2.0f * PI * Step / 32.0f;
        Points.Add(Centre + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Radius);
    }
    Stroke(Elements, Layer, Geometry, Points, Color, Thickness, true);
}

int32 UPuzzleBoardWidget::NativePaint(const FPaintArgs& Args, const FGeometry& Geometry,
    const FSlateRect& Culling, FSlateWindowElementList& Elements, int32 Layer,
    const FWidgetStyle& Style, bool bEnabled) const
{
    const int32 BaseLayer = Super::NativePaint(Args, Geometry, Culling, Elements, Layer, Style, bEnabled);
    if (!Puzzle || Puzzle->GetCells().IsEmpty()) return BaseLayer;

    const int32 BoardW = Puzzle->GetBoardWidth();
    const int32 BoardH = Puzzle->GetBoardHeight();
    const FVector2D Size = Geometry.GetLocalSize();
    const float Margin = 54.0f;
    const float Cell = FMath::Min((Size.X - Margin * 2) / BoardW, (Size.Y - Margin * 2) / BoardH);
    const FVector2D Origin((Size.X - Cell * BoardW) * 0.5f, (Size.Y - Cell * BoardH) * 0.5f);

    // Nothing is drawn behind the board: the projection is transparent wherever a piece or the
    // beam is not, and there is no lattice, so only the parts that affect the beam are visible.
    const TArray<FPuzzleCell>& Cells = Puzzle->GetCells();
    const int32 PieceLayer = BaseLayer + 1;
    const int32 BeamLayer = BaseLayer + 2;
    const int32 SelectLayer = BaseLayer + 3;

    for (int32 I = 0; I < Cells.Num(); ++I)
    {
        const FPuzzleCell& C = Cells[I];
        if (C.Type == EPuzzleCellType::Empty) continue;

        const int32 X = I % BoardW;
        const int32 Y = I / BoardW;
        // Pieces fill their square, so a mirror's hypotenuse runs corner to corner through the
        // cell centre: exactly the line the beam turns on.
        const float Inset = Cell * 0.06f;
        const FVector2D NW = Origin + FVector2D(X * Cell + Inset, Y * Cell + Inset);
        const FVector2D SE = Origin + FVector2D((X + 1) * Cell - Inset, (Y + 1) * Cell - Inset);
        const FVector2D NE(SE.X, NW.Y);
        const FVector2D SW(NW.X, SE.Y);
        const FVector2D Centre = Origin + FVector2D((X + 0.5f) * Cell, (Y + 0.5f) * Cell);

        switch (C.Type)
        {
        case EPuzzleCellType::Start:
        {
            // Points east, the way the beam leaves.
            const TArray<FVector2D> Tri = {FVector2D(SE.X, Centre.Y), NW, SW};
            Shape(Elements, PieceLayer, Geometry, Tri, BeamColor(EPuzzleLaserColor::Red), 3.0f);
            Hatch(Elements, PieceLayer, Geometry, Tri, BeamColor(EPuzzleLaserColor::Red) * 0.6f, 4.0f);
            break;
        }

        case EPuzzleCellType::End:
        {
            // The exit wears the colour it demands, so the target is readable before you solve it.
            const FLinearColor Goal = Puzzle->IsSolved()
                ? FLinearColor(0.2f, 1.0f, 0.4f, 1.0f) : BeamColor(C.Color);
            Ring(Elements, PieceLayer, Geometry, Centre, (SE.X - NW.X) * 0.48f, Goal, 3.0f);
            Ring(Elements, PieceLayer, Geometry, Centre, (SE.X - NW.X) * 0.30f, Goal * 0.8f, 2.0f);
            Ring(Elements, PieceLayer, Geometry, Centre, (SE.X - NW.X) * 0.12f, Goal, 3.0f);
            break;
        }

        case EPuzzleCellType::Mirror:
        {
            // Right angle at SE, SW, NW then NE as the rotation advances; the remaining edge is
            // the hypotenuse, and that is the surface the reflection rules use.
            TArray<FVector2D> Tri;
            FVector2D HypA, HypB;
            switch (C.Rotation & 3)
            {
            case 0:  Tri = {SW, NE, SE}; HypA = SW; HypB = NE; break;
            case 1:  Tri = {NW, SE, SW}; HypA = NW; HypB = SE; break;
            case 2:  Tri = {SW, NE, NW}; HypA = SW; HypB = NE; break;
            default: Tri = {NW, SE, NE}; HypA = NW; HypB = SE; break;
            }
            Shape(Elements, PieceLayer, Geometry, Tri, StructureColor, 2.0f);
            Hatch(Elements, PieceLayer, Geometry, Tri, StructureColor * 0.35f, 5.0f);
            // The mirrored face, picked out so it is obvious which way it bounces.
            Stroke(Elements, PieceLayer, Geometry, {HypA, HypB}, FLinearColor::White, 4.0f, true);
            break;
        }

        case EPuzzleCellType::Blocker:
        {
            const TArray<FVector2D> Box = {NW, NE, SE, SW};
            const FLinearColor Dim = FLinearColor(0.45f, 0.60f, 0.70f, 1.0f);
            Shape(Elements, PieceLayer, Geometry, Box, Dim, 2.0f);
            Hatch(Elements, PieceLayer, Geometry, Box, Dim * 0.5f, 5.0f);
            break;
        }

        case EPuzzleCellType::ColorFilter:
        {
            // Deliberately the blocker's outline without the hatching: an open square reads as
            // something the beam can get through, and its colour says which beam.
            const TArray<FVector2D> Box = {NW, NE, SE, SW};
            const FLinearColor Filter = BeamColor(C.Color);
            Shape(Elements, PieceLayer, Geometry, Box, Filter, 3.0f);
            const float Inner = (SE.X - NW.X) * 0.16f;
            const TArray<FVector2D> InnerBox = {
                NW + FVector2D(Inner, Inner), NE + FVector2D(-Inner, Inner),
                SE + FVector2D(-Inner, -Inner), SW + FVector2D(Inner, -Inner)};
            Shape(Elements, PieceLayer, Geometry, InnerBox, Filter * 0.5f, 1.5f);
            break;
        }

        case EPuzzleCellType::ColorSwitch:
        {
            const FLinearColor Gate = BeamColor(C.Color);
            const float R = (SE.X - NW.X) * 0.42f;
            Ring(Elements, PieceLayer, Geometry, Centre, R, C.bActive ? Gate : Gate * 0.45f,
                C.bActive ? 4.0f : 2.0f);
            if (C.bActive)
            {
                Ring(Elements, PieceLayer, Geometry, Centre, R * 0.5f, Gate, 3.0f);
            }
            break;
        }

        default: break;
        }
    }

    for (const FPuzzleLaserSegment& Segment : Puzzle->GetLaserPath())
    {
        const FVector2D A = Origin + FVector2D((Segment.From.X + .5f) * Cell, (Segment.From.Y + .5f) * Cell);
        const FVector2D B = Origin + FVector2D((Segment.To.X + .5f) * Cell, (Segment.To.Y + .5f) * Cell);
        const FLinearColor Colour = BeamColor(Segment.Color);
        Stroke(Elements, BeamLayer, Geometry, {A, B}, Colour, 5.0f, true);
        Stroke(Elements, BeamLayer, Geometry, {A, B}, FLinearColor(1.0f, 1.0f, 1.0f, 0.85f), 1.5f, false);
    }

    const int32 Selected = Puzzle->GetSelectedCell();
    if (Puzzle->IsFocused() && Cells.IsValidIndex(Selected))
    {
        const int32 X = Selected % BoardW;
        const int32 Y = Selected / BoardW;
        const FVector2D Min = Origin + FVector2D(X * Cell, Y * Cell);
        const FVector2D Max = Min + FVector2D(Cell, Cell);
        const float Arm = Cell * 0.30f;
        const FVector2D Corner[4] = {Min, FVector2D(Max.X, Min.Y), Max, FVector2D(Min.X, Max.Y)};
        const FVector2D Horz[4] = {FVector2D(1,0), FVector2D(-1,0), FVector2D(-1,0), FVector2D(1,0)};
        const FVector2D Vert[4] = {FVector2D(0,1), FVector2D(0,1), FVector2D(0,-1), FVector2D(0,-1)};
        // Brackets take a player's colour while that player is waiting for the other to agree,
        // so a half-entered input is visible rather than silently dropped.
        const int32 Pending = Puzzle->GetPendingInputMask();
        FLinearColor BracketColor = SelectionColor;
        if (Pending == 1) BracketColor = PlayerOneColor;
        else if (Pending == 2) BracketColor = PlayerTwoColor;

        for (int32 K = 0; K < 4; ++K)
        {
            Stroke(Elements, SelectLayer, Geometry, {Corner[K], Corner[K] + Horz[K] * Arm}, BracketColor, 3.0f, true);
            Stroke(Elements, SelectLayer, Geometry, {Corner[K], Corner[K] + Vert[K] * Arm}, BracketColor, 3.0f, true);
        }
    }

    return SelectLayer;
}
