#include "SharedHeroCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "CoopPlayerController.h"
#include "DamageNumberActor.h"
#include "DrawDebugHelpers.h"
#include "Engine/SkeletalMesh.h"
#include "GamejamRazigra.h"
#include "GlobalGameData.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

ASharedHeroCharacter::ASharedHeroCharacter()
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = true;
    SetReplicateMovement(true);
    SetNetUpdateFrequency(60.0f);

    GetCapsuleComponent()->InitCapsuleSize(42.0f, 96.0f);
    GetMesh()->SetRelativeLocationAndRotation(FVector(0.0f, 0.0f, -96.0f), FRotator(0.0f, -90.0f, 0.0f));
    GetCharacterMovement()->bRunPhysicsWithNoController = true;
    GetCharacterMovement()->bOrientRotationToMovement = false;
    GetCharacterMovement()->NavAgentProps.bCanCrouch = true;
    bUseControllerRotationYaw = false;

    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    CameraBoom->SetupAttachment(RootComponent);
    CameraBoom->TargetArmLength = 400.0f;
    CameraBoom->SocketOffset = FVector(0.0f, 55.0f, 70.0f);
    CameraBoom->bUsePawnControlRotation = false;
    CameraBoom->SetUsingAbsoluteRotation(true);
    CameraBoom->bEnableCameraLag = true;
    CameraBoom->CameraLagSpeed = 12.0f;

    FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
    FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
    FollowCamera->bUsePawnControlRotation = false;
}

void ASharedHeroCharacter::BeginPlay()
{
    Super::BeginPlay();

    const UGlobalGameData* Data = UGlobalGameData::Get(this);
    Health = Data->HeroMaxHealth;
    GetCharacterMovement()->MaxWalkSpeed = Data->WalkSpeed;
    GetCharacterMovement()->MaxWalkSpeedCrouched = Data->CrouchedSpeed;
    GetCharacterMovement()->JumpZVelocity = Data->JumpVelocity;

    EnsureVisibleMesh();

    AimRotation = FRotator(-10.0f, GetActorRotation().Yaw, 0.0f);
    OnRep_AimRotation();
}

/**
 * The hero's mesh belongs to its Blueprint now. If the game is running on the bare C++ class
 * (no Hero Blueprint configured yet) it would be completely invisible, so fall back to the
 * sample mannequin and say so rather than dropping the player into an empty level.
 */
void ASharedHeroCharacter::EnsureVisibleMesh()
{
    USkeletalMeshComponent* MeshComponent = GetMesh();
    if (!MeshComponent || MeshComponent->GetSkeletalMeshAsset())
    {
        return;
    }

    UE_LOG(LogRazigra, Warning,
        TEXT("The shared hero has no mesh. Set GlobalGameData.HeroBlueprint to a hero Blueprint "
             "(run the CreateRazigraData commandlet to generate /Game/Blueprints/BP_SharedHero). "
             "Falling back to the sample mannequin."));

    if (USkeletalMesh* FallbackMesh = LoadObject<USkeletalMesh>(nullptr,
        TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple")))
    {
        MeshComponent->SetSkeletalMeshAsset(FallbackMesh);
    }
    if (!MeshComponent->GetAnimInstance())
    {
        if (UClass* FallbackAnimation = LoadClass<UAnimInstance>(nullptr,
            TEXT("/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed.ABP_Unarmed_C")))
        {
            MeshComponent->SetAnimInstanceClass(FallbackAnimation);
        }
    }
}

void ASharedHeroCharacter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!HasAuthority() || bIsDead)
    {
        return;
    }

    ProcessLook();
    ProcessMovement();
    ProcessActions();

    if (bIsFiring && GetWorld()->GetTimeSeconds() >= FiringVisualUntil)
    {
        bIsFiring = false;
    }
}

bool ASharedHeroCharacter::HasConsensus(EConsensusAction Action) const
{
    const int32 ActionIndex = static_cast<int32>(Action);
    for (int32 Index = 0; Index < RequiredConsensusParticipants; ++Index)
    {
        if (!ParticipantActions[Index][ActionIndex])
        {
            return false;
        }
    }
    return true;
}

void ASharedHeroCharacter::SetRequiredConsensusParticipants(int32 NewRequiredCount)
{
    if (HasAuthority())
    {
        RequiredConsensusParticipants = FMath::Clamp(NewRequiredCount, 1, ParticipantCount);
    }
}

void ASharedHeroCharacter::SetParticipantAction(int32 ParticipantIndex, EConsensusAction Action, bool bPressed)
{
    if (!HasAuthority() || ParticipantIndex < 0 || ParticipantIndex >= ParticipantCount || Action == EConsensusAction::MAX)
    {
        return;
    }
    ParticipantActions[ParticipantIndex][static_cast<int32>(Action)] = bPressed;
    RefreshActionMasks();
}

void ASharedHeroCharacter::RefreshActionMasks()
{
    int32 Masks[ParticipantCount] = {};
    for (int32 Participant = 0; Participant < ParticipantCount; ++Participant)
    {
        for (int32 ActionIndex = 0; ActionIndex < ActionCount; ++ActionIndex)
        {
            if (ParticipantActions[Participant][ActionIndex])
            {
                Masks[Participant] |= 1 << ActionIndex;
            }
        }
    }
    PlayerOneActionMask = Masks[0];
    PlayerTwoActionMask = Masks[1];
}

FVector2D ASharedHeroCharacter::GetParticipantLookAxis(int32 ParticipantIndex) const
{
    switch (ParticipantIndex)
    {
    case 0: return PlayerOneLookAxis;
    case 1: return PlayerTwoLookAxis;
    default: return FVector2D::ZeroVector;
    }
}

int32 ASharedHeroCharacter::GetParticipantActionMask(int32 ParticipantIndex) const
{
    switch (ParticipantIndex)
    {
    case 0: return PlayerOneActionMask;
    case 1: return PlayerTwoActionMask;
    default: return 0;
    }
}

bool ASharedHeroCharacter::IsActionPressedBy(int32 ParticipantIndex, EConsensusAction Action) const
{
    if (Action == EConsensusAction::MAX)
    {
        return false;
    }
    return (GetParticipantActionMask(ParticipantIndex) & (1 << static_cast<int32>(Action))) != 0;
}

void ASharedHeroCharacter::SubmitParticipantLook(int32 ParticipantIndex, const FVector2D& LookDelta)
{
    if (!HasAuthority() || ParticipantIndex < 0 || ParticipantIndex >= ParticipantCount || LookDelta.IsNearlyZero())
    {
        return;
    }
    const double Now = GetWorld()->GetTimeSeconds();
    PendingLook[ParticipantIndex] = LookDelta.GetClampedToMaxSize(50.0f);
    LookReceivedAt[ParticipantIndex] = Now;
    bLookPending[ParticipantIndex] = true;
    // Held a little longer than the consensus grace so the HUD light does not strobe while
    // a player keeps the mouse moving.
    LookActiveUntil[ParticipantIndex] = Now + FMath::Max(
        UGlobalGameData::Get(this)->LookInputGraceSeconds, 0.15f);
}

void ASharedHeroCharacter::ResetParticipant(int32 ParticipantIndex)
{
    if (!HasAuthority() || ParticipantIndex < 0 || ParticipantIndex >= ParticipantCount)
    {
        return;
    }
    for (int32 ActionIndex = 0; ActionIndex < ActionCount; ++ActionIndex)
    {
        ParticipantActions[ParticipantIndex][ActionIndex] = false;
    }
    bLookPending[ParticipantIndex] = false;
    LookActiveUntil[ParticipantIndex] = 0.0;
    RefreshActionMasks();
}

void ASharedHeroCharacter::ProcessMovement()
{
    const float ForwardValue = (HasConsensus(EConsensusAction::MoveForward) ? 1.0f : 0.0f)
        - (HasConsensus(EConsensusAction::MoveBackward) ? 1.0f : 0.0f);
    const float RightValue = (HasConsensus(EConsensusAction::MoveRight) ? 1.0f : 0.0f)
        - (HasConsensus(EConsensusAction::MoveLeft) ? 1.0f : 0.0f);

    const FRotator YawOnly(0.0f, AimRotation.Yaw, 0.0f);
    AddMovementInput(FRotationMatrix(YawOnly).GetUnitAxis(EAxis::X), ForwardValue);
    AddMovementInput(FRotationMatrix(YawOnly).GetUnitAxis(EAxis::Y), RightValue);

    if (!FMath::IsNearlyZero(ForwardValue) || !FMath::IsNearlyZero(RightValue))
    {
        const FVector Direction = (FRotationMatrix(YawOnly).GetUnitAxis(EAxis::X) * ForwardValue
            + FRotationMatrix(YawOnly).GetUnitAxis(EAxis::Y) * RightValue).GetSafeNormal();
        SetActorRotation(FMath::RInterpTo(GetActorRotation(), Direction.Rotation(), GetWorld()->GetDeltaSeconds(), 12.0f));
    }
}

void ASharedHeroCharacter::ProcessLook()
{
    const UGlobalGameData* Data = UGlobalGameData::Get(this);
    const double Now = GetWorld()->GetTimeSeconds();

    for (int32 Index = 0; Index < ParticipantCount; ++Index)
    {
        if (bLookPending[Index] && Now - LookReceivedAt[Index] > Data->LookInputGraceSeconds)
        {
            bLookPending[Index] = false;
        }
    }

    // Publish who is steering, and by how much, so the HUD can light the card and drive the
    // axis meters. Both are cleared as soon as a player stops moving the mouse.
    const int32 LookActionIndex = static_cast<int32>(EConsensusAction::Look);
    FVector2D LiveAxes[ParticipantCount] = {};
    for (int32 Index = 0; Index < ParticipantCount; ++Index)
    {
        const bool bSteering = Now < LookActiveUntil[Index];
        ParticipantActions[Index][LookActionIndex] = bSteering;
        LiveAxes[Index] = bSteering ? PendingLook[Index] : FVector2D::ZeroVector;
    }
    PlayerOneLookAxis = LiveAxes[0];
    PlayerTwoLookAxis = LiveAxes[1];
    RefreshActionMasks();

    bool bAllLookInputsReady = true;
    FVector2D Combined = FVector2D::ZeroVector;
    for (int32 Index = 0; Index < RequiredConsensusParticipants; ++Index)
    {
        bAllLookInputsReady &= bLookPending[Index];
        Combined += PendingLook[Index];
    }

    if (bAllLookInputsReady)
    {
        // Both deltas are summed, not averaged: aiming is the two players' contributions added
        // together, so pulling in opposite directions cancels out.
        Combined *= Data->LookSensitivity;
        AimRotation.Yaw = FRotator::NormalizeAxis(AimRotation.Yaw + Combined.X);
        AimRotation.Pitch = FMath::Clamp(AimRotation.Pitch - Combined.Y, -70.0f, 60.0f);
        for (int32 Index = 0; Index < RequiredConsensusParticipants; ++Index)
        {
            bLookPending[Index] = false;
        }
        OnRep_AimRotation();
    }
}

void ASharedHeroCharacter::ProcessActions()
{
    const bool bCrouchConsensus = HasConsensus(EConsensusAction::Crouch);
    if (bCrouchConsensus && !bIsCrouched)
    {
        Crouch();
    }
    else if (!bCrouchConsensus && bIsCrouched)
    {
        UnCrouch();
    }

    const bool bJumpConsensus = HasConsensus(EConsensusAction::Jump);
    if (bJumpConsensus && !bWasJumpConsensus)
    {
        Jump();
    }
    if (!bJumpConsensus)
    {
        StopJumping();
    }
    bWasJumpConsensus = bJumpConsensus;

    if (HasConsensus(EConsensusAction::Fire) && GetWorld()->GetTimeSeconds() >= NextFireTime)
    {
        FireGun();
    }
}

void ASharedHeroCharacter::FireGun()
{
    const UGlobalGameData* Data = UGlobalGameData::Get(this);
    const double Now = GetWorld()->GetTimeSeconds();
    NextFireTime = Now + Data->FireInterval;
    FiringVisualUntil = Now + FMath::Min(Data->FireInterval, 0.12f);
    bIsFiring = true;

    // The HUD dot is screen center, which is the follow camera's forward vector.
    const FVector TraceStart = FollowCamera->GetComponentLocation();
    const FVector ShotDirection = FollowCamera->GetForwardVector();
    const FVector TraceEnd = TraceStart + ShotDirection * Data->FireRange;
    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(RazigraGun), true, this);
    const bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, Params);
    const FVector FinalEnd = bHit ? Hit.ImpactPoint : TraceEnd;
    AActor* HitActor = bHit ? Hit.GetActor() : nullptr;

    if (IsValid(HitActor))
    {
        UGameplayStatics::ApplyPointDamage(HitActor, Data->FireDamage, ShotDirection, Hit,
            nullptr, this, UDamageType::StaticClass());
    }
    UE_LOG(LogRazigra, Log, TEXT("Weapon fired: hit=%s actor=%s distance=%.0f."),
        bHit ? TEXT("true") : TEXT("false"), HitActor ? *HitActor->GetName() : TEXT("none"),
        FVector::Distance(TraceStart, FinalEnd));
    // Effects hang off the muzzle, not the camera, so a Blueprint child can attach them to the mesh.
    MulticastGunFired(GetMuzzleLocation(), FinalEnd, bHit, HitActor);
}

FVector ASharedHeroCharacter::GetMuzzleLocation() const
{
    const FName MuzzleSocket = UGlobalGameData::Get(this)->MuzzleSocketName;
    if (const USkeletalMeshComponent* MeshComponent = GetMesh())
    {
        if (!MuzzleSocket.IsNone() && MeshComponent->DoesSocketExist(MuzzleSocket))
        {
            return MeshComponent->GetSocketLocation(MuzzleSocket);
        }
    }
    return FollowCamera ? FollowCamera->GetComponentLocation() : GetActorLocation();
}

void ASharedHeroCharacter::MulticastGunFired_Implementation(const FVector_NetQuantize& MuzzleLocation,
    const FVector_NetQuantize& ImpactPoint, bool bHit, AActor* HitActor)
{
    BP_OnGunFired(MuzzleLocation, ImpactPoint, bHit, HitActor);
    DrawDebugLine(GetWorld(), MuzzleLocation, ImpactPoint,
        bHit ? FColor::Green : FColor::Orange, false, 0.10f, 0, 2.5f);
    DrawDebugSphere(GetWorld(), MuzzleLocation, 7.0f, 8, FColor::Yellow, false, 0.08f, 0, 1.5f);
    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        if (ACoopPlayerController* LocalController = Cast<ACoopPlayerController>(It->Get()))
        {
            if (LocalController->IsLocalController())
            {
                LocalController->HandleGunFired(bHit);
            }
        }
    }
}

float ASharedHeroCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent,
    AController* EventInstigator, AActor* DamageCauser)
{
    if (!HasAuthority() || bIsDead)
    {
        return 0.0f;
    }
    const float Applied = FMath::Min(Health, FMath::Max(0.0f, DamageAmount));
    Health -= Applied;
    if (Applied > 0.0f)
    {
        MulticastHeroDamaged(Applied);
    }
    if (Health <= 0.0f)
    {
        bIsDead = true;
        GetCharacterMovement()->DisableMovement();
        MulticastHeroDied();
    }
    return Applied;
}

void ASharedHeroCharacter::MulticastHeroDamaged_Implementation(float DamageAmount)
{
    BP_OnHeroDamaged(DamageAmount);
    if (!GetWorld())
    {
        return;
    }
    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        if (ACoopPlayerController* LocalCoopController = Cast<ACoopPlayerController>(It->Get()))
        {
            if (LocalCoopController->IsLocalController())
            {
                LocalCoopController->HandleHeroDamaged(DamageAmount);
                const UGlobalGameData* Data = UGlobalGameData::Get(this);
                FActorSpawnParameters Params;
                Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
                Params.ObjectFlags |= RF_Transient;
                const FVector NumberLocation = GetActorLocation() + FVector::UpVector * Data->DamageNumberHeight;
                if (ADamageNumberActor* Number = GetWorld()->SpawnActor<ADamageNumberActor>(
                    ADamageNumberActor::StaticClass(), NumberLocation, FRotator::ZeroRotator, Params))
                {
                    Number->InitializeDamageNumber(DamageAmount, LocalCoopController);
                }
            }
        }
    }
}

void ASharedHeroCharacter::MulticastHeroDied_Implementation()
{
    BP_OnHeroDied();
}

bool ASharedHeroCharacter::IsCharacterJumping() const
{
    return GetCharacterMovement()->IsFalling() && GetVelocity().Z > 0.0f;
}

bool ASharedHeroCharacter::IsCharacterFalling() const
{
    return GetCharacterMovement()->IsFalling();
}

bool ASharedHeroCharacter::IsCharacterMoving() const
{
    return GetVelocity().SizeSquared2D() > 25.0f;
}

float ASharedHeroCharacter::GetHealthNormalized() const
{
    const float MaxHealth = UGlobalGameData::Get(this)->HeroMaxHealth;
    return MaxHealth > 0.0f ? Health / MaxHealth : 0.0f;
}

void ASharedHeroCharacter::OnRep_AimRotation()
{
    CameraBoom->SetRelativeRotation(AimRotation);
}

void ASharedHeroCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ASharedHeroCharacter, AimRotation);
    DOREPLIFETIME(ASharedHeroCharacter, Health);
    DOREPLIFETIME(ASharedHeroCharacter, bIsFiring);
    DOREPLIFETIME(ASharedHeroCharacter, bIsDead);
    DOREPLIFETIME(ASharedHeroCharacter, RequiredConsensusParticipants);
    DOREPLIFETIME(ASharedHeroCharacter, PlayerOneActionMask);
    DOREPLIFETIME(ASharedHeroCharacter, PlayerTwoActionMask);
    DOREPLIFETIME(ASharedHeroCharacter, PlayerOneLookAxis);
    DOREPLIFETIME(ASharedHeroCharacter, PlayerTwoLookAxis);
}
