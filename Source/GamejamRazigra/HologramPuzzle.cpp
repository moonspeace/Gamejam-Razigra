#include "HologramPuzzle.h"

#include "Camera/CameraComponent.h"
#include "Components/SceneComponent.h"
#include "Components/WidgetComponent.h"
#include "Net/UnrealNetwork.h"
#include "PuzzleBoardWidget.h"
#include "SharedHeroCharacter.h"

namespace
{
    FIntPoint Step(EPuzzleDirection Direction)
    {
        switch (Direction)
        {
        case EPuzzleDirection::North: return FIntPoint(0, -1);
        case EPuzzleDirection::East:  return FIntPoint(1, 0);
        case EPuzzleDirection::South: return FIntPoint(0, 1);
        default:                      return FIntPoint(-1, 0);
        }
    }

    /**
     * A mirror is a right triangle. It reflects off the hypotenuse and blocks anything that
     * arrives at its solid back, so the face you can see is the face that bounces the beam and
     * all four rotations behave differently. Rotation 0-3 puts the right angle at SE, SW, NW, NE,
     * which is exactly how the widget draws it.
     */
    bool Reflect(EPuzzleDirection Direction, uint8 Rotation, EPuzzleDirection& Out)
    {
        switch (Rotation & 3)
        {
        case 0: // "/" hypotenuse, solid corner south-east
            if (Direction == EPuzzleDirection::South) { Out = EPuzzleDirection::West;  return true; }
            if (Direction == EPuzzleDirection::East)  { Out = EPuzzleDirection::North; return true; }
            break;
        case 1: // "\" hypotenuse, solid corner south-west
            if (Direction == EPuzzleDirection::South) { Out = EPuzzleDirection::East;  return true; }
            if (Direction == EPuzzleDirection::West)  { Out = EPuzzleDirection::North; return true; }
            break;
        case 2: // "/" hypotenuse, solid corner north-west
            if (Direction == EPuzzleDirection::North) { Out = EPuzzleDirection::East;  return true; }
            if (Direction == EPuzzleDirection::West)  { Out = EPuzzleDirection::South; return true; }
            break;
        default: // "\" hypotenuse, solid corner north-east
            if (Direction == EPuzzleDirection::North) { Out = EPuzzleDirection::West;  return true; }
            if (Direction == EPuzzleDirection::East)  { Out = EPuzzleDirection::South; return true; }
            break;
        }
        return false;
    }
}

AHologramPuzzle::AHologramPuzzle()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;
    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);

    Projection = CreateDefaultSubobject<UWidgetComponent>(TEXT("HolographicProjection"));
    Projection->SetupAttachment(Root);
    Projection->SetWidgetSpace(EWidgetSpace::World);
    Projection->SetDrawSize(FVector2D(900, 900));
    Projection->SetPivot(FVector2D(0.5f, 0.5f));
    Projection->SetTwoSided(true);
    Projection->SetBlendMode(EWidgetBlendMode::Transparent);
    Projection->SetBackgroundColor(FLinearColor::Transparent);
    Projection->SetRelativeRotation(FRotator(0, 90, 0));

    PuzzleCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("PuzzleCamera"));
    PuzzleCamera->SetupAttachment(Root);
    PuzzleCamera->SetRelativeLocation(FVector(400, 0, 0));
    PuzzleCamera->SetRelativeRotation(FRotator(0, 180, 0));
    PuzzleCamera->FieldOfView = 55.0f;
}

void AHologramPuzzle::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    ApplyProjectionTint();
}

void AHologramPuzzle::ApplyProjectionTint()
{
    if (!Projection) return;
    // The render target is 8-bit, so colours are clamped there; the tint multiply happens
    // afterwards in the material, which is what actually lifts the board into HDR for bloom.
    const float Gain = FMath::Max(1.0f, BloomIntensity);
    Projection->SetTintColorAndOpacity(FLinearColor(
        HologramTint.R * Gain, HologramTint.G * Gain, HologramTint.B * Gain, HologramTint.A));
}

void AHologramPuzzle::BeginPlay()
{
    Super::BeginPlay();
    ApplyProjectionTint();
    if (HasAuthority()) BuildPuzzle();
    BoardWidget = CreateWidget<UPuzzleBoardWidget>(GetWorld(), UPuzzleBoardWidget::StaticClass());
    Projection->SetWidget(BoardWidget);
    RefreshWidget();
}

void AHologramPuzzle::BuildPuzzle()
{
    PuzzleId = FMath::Clamp(PuzzleId, 0, 2);
    BoardWidth = BoardHeight = PuzzleId == 0 ? 5 : (PuzzleId == 1 ? 7 : 9);
    Cells.SetNum(BoardWidth * BoardHeight);
    auto Put = [this](int32 X, int32 Y, EPuzzleCellType Type, uint8 Rotation = 0,
        EPuzzleLaserColor Color = EPuzzleLaserColor::Red)
    {
        FPuzzleCell& Cell = Cells[Index(X, Y)];
        Cell.Type = Type; Cell.Rotation = Rotation; Cell.Color = Color;
    };

    // Start bottom-left firing east, goal top-right. Every layout below was brute-forced
    // against the reflection rules above: each has exactly one solution and starts unsolved.
    // Start bottom-left firing east, goal top-right. Each layout was brute-forced against the
    // rules above, including the exit colour and the filters: one solution each, none pre-solved.
    Put(0, BoardHeight - 1, EPuzzleCellType::Start, 0, EPuzzleLaserColor::Red);

    if (PuzzleId == 0)
    {
        Put(4, 0, EPuzzleCellType::End, 0, EPuzzleLaserColor::Green);
        Put(2, 4, EPuzzleCellType::Mirror, 1);
        Put(2, 1, EPuzzleCellType::Mirror, 0);
        Put(4, 1, EPuzzleCellType::Mirror, 3);
        Put(1, 4, EPuzzleCellType::ColorSwitch, 0, EPuzzleLaserColor::Green);
        Put(2, 2, EPuzzleCellType::ColorFilter, 0, EPuzzleLaserColor::Green);
        Put(3, 2, EPuzzleCellType::Blocker);
    }
    else if (PuzzleId == 1)
    {
        Put(6, 0, EPuzzleCellType::End, 0, EPuzzleLaserColor::Blue);
        Put(3, 6, EPuzzleCellType::Mirror, 2);
        Put(3, 2, EPuzzleCellType::Mirror, 0);
        Put(6, 2, EPuzzleCellType::Mirror, 1);
        Put(1, 6, EPuzzleCellType::ColorSwitch, 0, EPuzzleLaserColor::Green);
        Put(3, 4, EPuzzleCellType::ColorSwitch, 0, EPuzzleLaserColor::Blue);
        Put(3, 5, EPuzzleCellType::ColorFilter, 0, EPuzzleLaserColor::Green);
        Put(5, 6, EPuzzleCellType::Blocker);
        Put(1, 2, EPuzzleCellType::Blocker);
    }
    else
    {
        Put(8, 0, EPuzzleCellType::End, 0, EPuzzleLaserColor::Red);
        Put(2, 8, EPuzzleCellType::Mirror, 3);
        Put(2, 5, EPuzzleCellType::Mirror, 0);
        Put(5, 5, EPuzzleCellType::Mirror, 1);
        Put(5, 7, EPuzzleCellType::Mirror, 2);
        Put(8, 7, EPuzzleCellType::Mirror, 3);
        Put(1, 8, EPuzzleCellType::ColorSwitch, 0, EPuzzleLaserColor::Green);
        Put(3, 5, EPuzzleCellType::ColorSwitch, 0, EPuzzleLaserColor::Blue);
        Put(6, 7, EPuzzleCellType::ColorSwitch, 0, EPuzzleLaserColor::Red);
        Put(2, 6, EPuzzleCellType::ColorFilter, 0, EPuzzleLaserColor::Green);
        Put(4, 5, EPuzzleCellType::ColorFilter, 0, EPuzzleLaserColor::Blue);
        Put(4, 8, EPuzzleCellType::Blocker);
        Put(2, 2, EPuzzleCellType::Blocker);
        Put(7, 5, EPuzzleCellType::Blocker);
    }

    SelectedCell = INDEX_NONE;
    for (int32 I = 0; I < Cells.Num(); ++I) if (IsInteractive(I)) { SelectedCell = I; break; }
    RecomputeLaser();
    ForceNetUpdate();
}

bool AHologramPuzzle::IsWithinInteractionRange(const ASharedHeroCharacter* Hero) const
{
    return Hero && FVector::DistSquared(Hero->GetActorLocation(), GetActorLocation())
        <= FMath::Square(InteractionDistance);
}

bool AHologramPuzzle::IsInteractive(int32 CellIndex) const
{
    return Cells.IsValidIndex(CellIndex) &&
        (Cells[CellIndex].Type == EPuzzleCellType::Mirror || Cells[CellIndex].Type == EPuzzleCellType::ColorSwitch);
}

void AHologramPuzzle::SetFocused(bool bNewFocused)
{
    if (!HasAuthority() || bSolved || bFocused == bNewFocused) return;
    bFocused = bNewFocused;
    PendingInputTime[0] = 0.0;
    PendingInputTime[1] = 0.0;
    PendingInputMask = 0;
    OnRep_State();
    ForceNetUpdate();
}

/**
 * Collects one player's request and only acts once the other asks for the same thing inside
 * ConsensusToleranceSeconds. The pair steers the board the same way they steer the hero: nothing
 * moves until both of them want it.
 */
void AHologramPuzzle::SubmitInput(int32 ParticipantIndex, EPuzzleInput Input)
{
    if (!HasAuthority() || !bFocused || bSolved
        || ParticipantIndex < 0 || ParticipantIndex >= PuzzleParticipantCount)
    {
        return;
    }

    const bool bNeedsConsensus = bRequireConsensusForSelection || Input == EPuzzleInput::Activate;
    if (!bNeedsConsensus)
    {
        RunInput(Input);
        return;
    }

    const double Now = GetWorld()->GetTimeSeconds();
    const int32 Other = 1 - ParticipantIndex;
    const bool bOtherAgrees = PendingInput[Other] == Input
        && (Now - PendingInputTime[Other]) <= ConsensusToleranceSeconds;

    if (bOtherAgrees)
    {
        // Both asked for it: spend both presses so a single hold cannot repeat the action.
        PendingInputTime[0] = 0.0;
        PendingInputTime[1] = 0.0;
        RefreshPendingMask();
        RunInput(Input);
        return;
    }

    PendingInput[ParticipantIndex] = Input;
    PendingInputTime[ParticipantIndex] = Now;
    RefreshPendingMask();
    OnRep_State();
    ForceNetUpdate();
}

/** Publishes who is still waiting on whom so the board can show it. */
void AHologramPuzzle::RefreshPendingMask()
{
    const double Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
    int32 Mask = 0;
    for (int32 Index = 0; Index < PuzzleParticipantCount; ++Index)
    {
        if (PendingInputTime[Index] > 0.0 && (Now - PendingInputTime[Index]) <= ConsensusToleranceSeconds)
        {
            Mask |= 1 << Index;
        }
    }
    PendingInputMask = Mask;
}

void AHologramPuzzle::RunInput(EPuzzleInput Input)
{
    switch (Input)
    {
    case EPuzzleInput::MoveNorth: MoveSelection(EPuzzleDirection::North); break;
    case EPuzzleInput::MoveEast:  MoveSelection(EPuzzleDirection::East);  break;
    case EPuzzleInput::MoveSouth: MoveSelection(EPuzzleDirection::South); break;
    case EPuzzleInput::MoveWest:  MoveSelection(EPuzzleDirection::West);  break;
    default:                      ActivateSelection();                    break;
    }
}

void AHologramPuzzle::MoveSelection(EPuzzleDirection Direction)
{
    if (!HasAuthority() || !bFocused || bSolved || SelectedCell == INDEX_NONE) return;
    const int32 X = SelectedCell % BoardWidth;
    const int32 Y = SelectedCell / BoardWidth;
    const FIntPoint Delta = Step(Direction);
    for (int32 D = 1; D <= FMath::Max(BoardWidth, BoardHeight); ++D)
    {
        const int32 NX = X + Delta.X * D, NY = Y + Delta.Y * D;
        if (NX < 0 || NY < 0 || NX >= BoardWidth || NY >= BoardHeight) break;
        const int32 Candidate = Index(NX, NY);
        if (IsInteractive(Candidate)) { SelectedCell = Candidate; OnRep_State(); ForceNetUpdate(); return; }
    }
}

void AHologramPuzzle::ActivateSelection()
{
    if (!HasAuthority() || !bFocused || bSolved || !IsInteractive(SelectedCell)) return;
    FPuzzleCell& Cell = Cells[SelectedCell];
    if (Cell.Type == EPuzzleCellType::Mirror) Cell.Rotation = (Cell.Rotation + 1) & 3;
    else Cell.bActive = !Cell.bActive;
    RecomputeLaser();
    if (TraceLaser(LaserPath)) { bSolved = true; bFocused = false; MulticastSolved(); }
    OnRep_State();
    ForceNetUpdate();
}

void AHologramPuzzle::RecomputeLaser() { TraceLaser(LaserPath); RefreshWidget(); }

bool AHologramPuzzle::TraceLaser(TArray<FPuzzleLaserSegment>& OutPath) const
{
    OutPath.Reset();
    int32 StartIndex = Cells.IndexOfByPredicate([](const FPuzzleCell& C){ return C.Type == EPuzzleCellType::Start; });
    if (StartIndex == INDEX_NONE) return false;
    FIntPoint P(StartIndex % BoardWidth, StartIndex / BoardWidth);
    EPuzzleDirection Direction = EPuzzleDirection::East;
    EPuzzleLaserColor Color = EPuzzleLaserColor::Red;
    TSet<int32> Visited;
    for (int32 Steps = 0; Steps < BoardWidth * BoardHeight * 4; ++Steps)
    {
        const FIntPoint Next = P + Step(Direction);
        if (Next.X < 0 || Next.Y < 0 || Next.X >= BoardWidth || Next.Y >= BoardHeight) return false;
        OutPath.Add({P, Next, Color});
        P = Next;
        const int32 I = Index(P.X, P.Y);
        const int32 StateKey = I * 16 + static_cast<int32>(Direction) * 4 + static_cast<int32>(Color);
        if (Visited.Contains(StateKey)) return false;
        Visited.Add(StateKey);
        const FPuzzleCell& Cell = Cells[I];
        // The exit only accepts its own colour now, so the switches are part of the solution
        // rather than decoration.
        if (Cell.Type == EPuzzleCellType::End) return Color == Cell.Color;
        if (Cell.Type == EPuzzleCellType::Blocker) return false;
        // Open square: its own colour passes straight through, anything else stops here.
        if (Cell.Type == EPuzzleCellType::ColorFilter && Color != Cell.Color) return false;
        if (Cell.Type == EPuzzleCellType::Mirror)
        {
            EPuzzleDirection Bounced;
            if (!Reflect(Direction, Cell.Rotation, Bounced)) return false;   // hit the solid back
            Direction = Bounced;
        }
        if (Cell.Type == EPuzzleCellType::ColorSwitch && Cell.bActive) Color = Cell.Color;
    }
    return false;
}

void AHologramPuzzle::RefreshWidget()
{
    if (!BoardWidget) return;
    BoardWidget->GlowStrength = FMath::Max(0.0f, BloomIntensity * 0.5f);
    BoardWidget->SetPuzzle(this);
}

void AHologramPuzzle::OnRep_State()
{
    BoardWidth = BoardHeight = PuzzleId == 0 ? 5 : (PuzzleId == 1 ? 7 : 9);
    RecomputeLaser();
}

void AHologramPuzzle::MulticastSolved_Implementation()
{
    BP_OnPuzzleSolved();
    RefreshWidget();
}

void AHologramPuzzle::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AHologramPuzzle, Cells);
    DOREPLIFETIME(AHologramPuzzle, SelectedCell);
    DOREPLIFETIME(AHologramPuzzle, bFocused);
    DOREPLIFETIME(AHologramPuzzle, bSolved);
}
