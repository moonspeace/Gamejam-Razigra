#include "SharedHeroCharacter.h"

#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextBlock.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "CoopPlayerController.h"
#include "DamageNumberActor.h"
#include "LaserTraceActor.h"
#include "Engine/SkeletalMesh.h"
#include "GamejamRazigra.h"
#include "GlobalGameData.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Net/UnrealNetwork.h"

ASharedHeroCharacter::ASharedHeroCharacter()
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = true;
    SetReplicateMovement(true);
    SetNetUpdateFrequency(60.0f);
    SetCanBeDamaged(true);

    GetCapsuleComponent()->InitCapsuleSize(42.0f, 96.0f);
    DefaultPawnCollisionResponse = GetCapsuleComponent()->GetCollisionResponseToChannel(ECC_Pawn);
    GetMesh()->SetRelativeLocationAndRotation(FVector(0.0f, 0.0f, -96.0f), FRotator(0.0f, -90.0f, 0.0f));
    GetCharacterMovement()->bRunPhysicsWithNoController = true;
    // The hero faces wherever the players are aiming and strafes around that, so character
    // movement must not swing it round to face its own velocity.
    GetCharacterMovement()->bOrientRotationToMovement = false;
    GetCharacterMovement()->bUseControllerDesiredRotation = false;
    GetCharacterMovement()->RotationRate = FRotator(0.0f, TurnRateDegreesPerSecond, 0.0f);
    GetCharacterMovement()->NavAgentProps.bCanCrouch = true;
    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = false;
    bUseControllerRotationRoll = false;

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

    ShieldMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ShieldMesh"));
    ShieldMesh->SetupAttachment(RootComponent);
    ShieldMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    ShieldMesh->SetCastShadow(false);
    ShieldMesh->SetVisibility(false);
    for (int32 Index = 0; Index < 6; ++Index)
    {
        UStaticMeshComponent* PlusPiece = CreateDefaultSubobject<UStaticMeshComponent>(
            *FString::Printf(TEXT("HealingPlusPiece%d"), Index));
        PlusPiece->SetupAttachment(RootComponent);
        PlusPiece->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        PlusPiece->SetCastShadow(false);
        PlusPiece->SetVisibility(false);
        HealingEffectMeshes.Add(PlusPiece);
    }
}

void ASharedHeroCharacter::BeginPlay()
{
    Super::BeginPlay();

    const UGlobalGameData* Data = UGlobalGameData::Get(this);
    Health = Data->HeroMaxHealth;
    GetCharacterMovement()->MaxWalkSpeed = Data->WalkSpeed;
    GetCharacterMovement()->MaxWalkSpeedCrouched = Data->CrouchedSpeed;
    GetCharacterMovement()->JumpZVelocity = Data->JumpVelocity;
    GetCharacterMovement()->RotationRate = FRotator(0.0f, TurnRateDegreesPerSecond, 0.0f);

    EnsureVisibleMesh();
    ConfigureCamera();
    ConfigureAbilityVisuals();

    AimRotation = FRotator(-10.0f, GetActorRotation().Yaw, 0.0f);
    OnRep_AimRotation();
}

void ASharedHeroCharacter::ConfigureCamera()
{
    const UGlobalGameData* Data = UGlobalGameData::Get(this);
    const FName AttachBone = Data->CameraAttachBoneName;
    if (!AttachBone.IsNone() && GetMesh() && GetMesh()->DoesSocketExist(AttachBone))
    {
        CameraBoom->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, AttachBone);
        CameraBoom->SetRelativeLocation(Data->CameraOffset);
        CameraBoom->SocketOffset = FVector::ZeroVector;
        UE_LOG(LogRazigra, Log, TEXT("Camera boom attached to hero bone/socket '%s' with offset %s."),
            *AttachBone.ToString(), *Data->CameraOffset.ToCompactString());
        return;
    }

    CameraBoom->AttachToComponent(GetCapsuleComponent(), FAttachmentTransformRules::KeepRelativeTransform);
    CameraBoom->SetRelativeLocation(FVector::ZeroVector);
    CameraBoom->SocketOffset = Data->CameraOffset;
    if (!AttachBone.IsNone())
    {
        UE_LOG(LogRazigra, Warning, TEXT("Camera bone/socket '%s' was not found; using root-mounted camera."),
            *AttachBone.ToString());
    }
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
    AbilityVisualTime += DeltaSeconds;
    if (bHealingActive && HealingEffectMeshes.Num() == 6)
    {
        const UGlobalGameData* Data = UGlobalGameData::Get(this);
        const FVector Centers[] = { FVector(-42, 0, 5), FVector(38, 8, 28), FVector(0, -15, 62) };
        APlayerCameraManager* Camera = GetWorld()->GetFirstPlayerController()
            ? GetWorld()->GetFirstPlayerController()->PlayerCameraManager : nullptr;
        for (int32 PlusIndex = 0; PlusIndex < 3; ++PlusIndex)
        {
            const float Phase = FMath::Fmod(AbilityVisualTime * 0.75f + PlusIndex / 3.0f, 1.0f);
            const float FadeScale = FMath::Clamp((1.0f - Phase) * 2.5f, 0.0f, 1.0f);
            const FVector LocalPosition = Data->HealingEffectOffset + Centers[PlusIndex]
                + FVector::UpVector * Phase * 95.0f;
            const float S = Data->HealingPlusSize / 100.0f * FadeScale;
            for (int32 Piece = 0; Piece < 2; ++Piece)
            {
                UStaticMeshComponent* PlusPiece = HealingEffectMeshes[PlusIndex * 2 + Piece];
                PlusPiece->SetRelativeLocation(LocalPosition);
                PlusPiece->SetRelativeScale3D(Piece == 0
                    ? FVector(S * 0.28f, S * 0.09f, S)
                    : FVector(S, S * 0.09f, S * 0.28f));
                if (Camera)
                {
                    const FVector Facing = Camera->GetCameraLocation() - PlusPiece->GetComponentLocation();
                    const FRotator TargetRotation = FRotationMatrix::MakeFromY(Facing.GetSafeNormal()).Rotator();
                    PlusPiece->SetWorldRotation(FMath::RInterpTo(
                        PlusPiece->GetComponentRotation(), TargetRotation, DeltaSeconds, 8.0f));
                }
            }
        }
    }
    if (!HasAuthority() || bIsDead)
    {
        return;
    }

    ProcessLook();
    ProcessMovement();
    ProcessActions();

    if (bHealingActive)
    {
        const UGlobalGameData* Data = UGlobalGameData::Get(this);
        Health = FMath::Min(Data->HeroMaxHealth, Health + Data->HealingPerSecond * DeltaSeconds);
    }

    UpdateRecoilHeat(DeltaSeconds);

    if (bIsFiring && GetWorld()->GetTimeSeconds() >= FiringVisualUntil)
    {
        bIsFiring = false;
    }
}

/**
 * The bar fills a notch per shot and bleeds back down whenever the trigger is not held, so
 * holding it never recovers. Filling it locks the weapon out; during the lockout the bar drains
 * across the whole duration, which doubles as the countdown the players watch.
 */
void ASharedHeroCharacter::UpdateRecoilHeat(float DeltaSeconds)
{
    const UGlobalGameData* Data = UGlobalGameData::Get(this);
    const double Now = GetWorld()->GetTimeSeconds();

    if (bWeaponOverheated)
    {
        const float Remaining = static_cast<float>(FMath::Max(0.0, OverheatUntil - Now));
        RecoilHeat = Data->RecoilOverheatSeconds > 0.0f
            ? FMath::Clamp(Remaining / Data->RecoilOverheatSeconds, 0.0f, 1.0f) : 0.0f;
        if (Now >= OverheatUntil)
        {
            bWeaponOverheated = false;
            RecoilHeat = 0.0f;
        }
        return;
    }

    if (bShieldActive)
    {
        RecoilHeat = FMath::Min(1.0f, RecoilHeat + Data->ShieldHeatPerSecond * DeltaSeconds);
        if (RecoilHeat >= 1.0f)
        {
            bShieldActive = false;
            OnRep_AbilityState();
            TriggerWeaponOverheat();
        }
    }
    else if (!HasConsensus(EConsensusAction::Fire) && RecoilHeat > 0.0f)
    {
        RecoilHeat = FMath::Max(0.0f, RecoilHeat - Data->RecoilCooldownPerSecond * DeltaSeconds);
    }
}

void ASharedHeroCharacter::TriggerWeaponOverheat()
{
    if (bWeaponOverheated) return;
    bWeaponOverheated = true;
    RecoilHeat = 1.0f;
    OverheatUntil = GetWorld()->GetTimeSeconds() + UGlobalGameData::Get(this)->RecoilOverheatSeconds;
    MulticastWeaponOverheated();
    ForceNetUpdate();
}

void ASharedHeroCharacter::MulticastWeaponOverheated_Implementation()
{
    BP_OnWeaponOverheated();
}

void ASharedHeroCharacter::SetParticipantAbility(int32 ParticipantIndex, bool bPressed)
{
    if (!HasAuthority() || ParticipantIndex < 0 || ParticipantIndex >= ParticipantCount || bIsDead)
    {
        return;
    }
    if (bPressed)
    {
        // One ability between the two of them: whoever gets there first keeps it until they let
        // go, so the pair never has healing and the shield up at the same time.
        const bool bOtherAbilityHeld = (ParticipantIndex == 0) ? bShieldActive : bHealingActive;
        if (bOtherAbilityHeld)
        {
            return;
        }
        if (ParticipantIndex == 0) bHealingActive = true;
        if (ParticipantIndex == 1)
        {
            // Shield and gun share one heat budget; an overheated character cannot raise it.
            if (bWeaponOverheated || RecoilHeat >= 1.0f) return;
            bShieldActive = true;
        }
        ParticipantActions[ParticipantIndex][static_cast<int32>(EConsensusAction::Fire)] = false;
        RefreshActionMasks();
    }
    else
    {
        if (ParticipantIndex == 0) bHealingActive = false;
        if (ParticipantIndex == 1) bShieldActive = false;
    }
    OnRep_AbilityState();
    ForceNetUpdate();
}

void ASharedHeroCharacter::ConfigureAbilityVisuals()
{
    const UGlobalGameData* Data = UGlobalGameData::Get(this);
    if (UStaticMesh* Sphere = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere")))
    {
        ShieldMesh->SetStaticMesh(Sphere);
        ShieldMesh->SetRelativeScale3D(FVector(Data->ShieldRadius / 50.0f));
    }
    UMaterialInterface* Base = Data->ShieldMaterial.LoadSynchronous();
    if (!Base)
    {
        Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materials/M_PlayerShield.M_PlayerShield"));
    }
    if (Base)
    {
        ShieldDynamicMaterial = UMaterialInstanceDynamic::Create(Base, this);
        ShieldDynamicMaterial->SetVectorParameterValue(TEXT("EffectColor"), Data->ShieldColor);
        ShieldDynamicMaterial->SetScalarParameterValue(TEXT("Intensity"), Data->ShieldEmissiveIntensity);
        ShieldDynamicMaterial->SetScalarParameterValue(TEXT("Opacity"), Data->ShieldColor.A);
        ShieldMesh->SetMaterial(0, ShieldDynamicMaterial);
    }
    if (UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")))
    {
        const float S = Data->HealingPlusSize / 100.0f;
        const FVector Centers[] = { FVector(-42, 0, 5), FVector(38, 8, 28), FVector(0, -15, 62) };
        for (int32 PlusIndex = 0; PlusIndex < 3; ++PlusIndex)
        {
            UStaticMeshComponent* Vertical = HealingEffectMeshes[PlusIndex * 2];
            UStaticMeshComponent* Horizontal = HealingEffectMeshes[PlusIndex * 2 + 1];
            Vertical->SetStaticMesh(Cube);
            Horizontal->SetStaticMesh(Cube);
            Vertical->SetRelativeLocation(Data->HealingEffectOffset + Centers[PlusIndex]);
            Horizontal->SetRelativeLocation(Data->HealingEffectOffset + Centers[PlusIndex]);
            Vertical->SetRelativeScale3D(FVector(S * 0.28f, S * 0.09f, S));
            Horizontal->SetRelativeScale3D(FVector(S, S * 0.09f, S * 0.28f));
        }
    }
    UMaterialInterface* HealingBase = Data->HealingEffectMaterial.LoadSynchronous();
    if (!HealingBase)
    {
        HealingBase = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materials/M_HealingPlus.M_HealingPlus"));
    }
    if (HealingBase)
    {
        HealingDynamicMaterial = UMaterialInstanceDynamic::Create(HealingBase, this);
        HealingDynamicMaterial->SetVectorParameterValue(TEXT("EffectColor"), Data->HealingEffectColor);
        HealingDynamicMaterial->SetScalarParameterValue(TEXT("Intensity"), Data->HealingEffectIntensity);
        HealingDynamicMaterial->SetScalarParameterValue(TEXT("Opacity"), 1.0f);
        for (UStaticMeshComponent* PlusPiece : HealingEffectMeshes)
        {
            PlusPiece->SetMaterial(0, HealingDynamicMaterial);
        }
    }
    OnRep_AbilityState();
}

void ASharedHeroCharacter::OnRep_AbilityState()
{
    for (UStaticMeshComponent* PlusPiece : HealingEffectMeshes)
    {
        PlusPiece->SetVisibility(bHealingActive, true);
    }
    ShieldMesh->SetVisibility(bShieldActive, true);
    // Zombies use the Pawn object channel. Ignoring that channel on the hero makes collision
    // non-blocking in both directions for every existing and newly spawned zombie, without
    // disabling zombie/world collision (which would make them fall through the level).
    if (UCapsuleComponent* HeroCapsule = GetCapsuleComponent())
    {
        HeroCapsule->SetCollisionResponseToChannel(ECC_Pawn,
            bShieldActive ? ECR_Ignore : DefaultPawnCollisionResponse.GetValue());
    }
    BP_OnHealingStateChanged(bHealingActive);
    BP_OnShieldStateChanged(bShieldActive);
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
    if (ParticipantIndex == 0) bHealingActive = false;
    if (ParticipantIndex == 1) bShieldActive = false;
    OnRep_AbilityState();
    RefreshActionMasks();
}

void ASharedHeroCharacter::ProcessMovement()
{
    const float ForwardValue = (HasConsensus(EConsensusAction::MoveForward) ? 1.0f : 0.0f)
        - (HasConsensus(EConsensusAction::MoveBackward) ? 1.0f : 0.0f);
    const float RightValue = (HasConsensus(EConsensusAction::MoveRight) ? 1.0f : 0.0f)
        - (HasConsensus(EConsensusAction::MoveLeft) ? 1.0f : 0.0f);

    if (bHealingActive)
    {
        // Healing plants the hero. Only horizontal motion is cancelled, so a
        // player who triggers one mid-air still falls instead of freezing in the sky.
        if (GetCharacterMovement()->IsMovingOnGround())
        {
            GetCharacterMovement()->Velocity.X = 0.0f;
            GetCharacterMovement()->Velocity.Y = 0.0f;
        }
        return;
    }

    const FRotator YawOnly(0.0f, AimRotation.Yaw, 0.0f);
    AddMovementInput(FRotationMatrix(YawOnly).GetUnitAxis(EAxis::X), ForwardValue);
    AddMovementInput(FRotationMatrix(YawOnly).GetUnitAxis(EAxis::Y), RightValue);

    // Face the aim at all times, moving or not. The input above is relative to this same yaw,
    // so W walks toward the crosshair and A/D strafe around it rather than turning into it.
    // That separation is what gives the locomotion blend space a Direction worth blending:
    // facing the velocity would pin Direction at zero and only ever play the forward animation.
    const FRotator Facing = FMath::RInterpConstantTo(GetActorRotation(), YawOnly,
        GetWorld()->GetDeltaSeconds(), TurnRateDegreesPerSecond);
    SetActorRotation(FRotator(0.0f, Facing.Yaw, 0.0f));
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
        if (bIsCrouched)
        {
            CrouchDamageImmunityUntil = GetWorld()->GetTimeSeconds()
                + UGlobalGameData::Get(this)->CrouchDamageImmunitySeconds;
        }
    }
    else if (!bCrouchConsensus && bIsCrouched)
    {
        UnCrouch();
    }

    const bool bJumpConsensus = HasConsensus(EConsensusAction::Jump) && !IsAbilityRooting();
    if (bJumpConsensus && !bWasJumpConsensus)
    {
        Jump();
    }
    if (!bJumpConsensus)
    {
        StopJumping();
    }
    bWasJumpConsensus = bJumpConsensus;

    if (!bWeaponOverheated && !bHealingActive && !bShieldActive && HasConsensus(EConsensusAction::Fire)
        && GetWorld()->GetTimeSeconds() >= NextFireTime)
    {
        FireGun();
    }
}

bool ASharedHeroCharacter::IsCrouchDamageImmunityActive() const
{
    return bIsCrouched && GetWorld()
        && GetWorld()->GetTimeSeconds() < CrouchDamageImmunityUntil;
}

void ASharedHeroCharacter::FireGun()
{
    const UGlobalGameData* Data = UGlobalGameData::Get(this);
    const double Now = GetWorld()->GetTimeSeconds();
    NextFireTime = Now + Data->FireInterval;

    RecoilHeat = FMath::Clamp(RecoilHeat + Data->RecoilHeatPerShot, 0.0f, 1.0f);
    if (RecoilHeat >= 1.0f) TriggerWeaponOverheat();
    FiringVisualUntil = Now + FMath::Min(Data->FireInterval, 0.12f);
    bIsFiring = true;

    // The HUD dot is screen center, which is the follow camera's forward vector.
    const FVector TraceStart = FollowCamera->GetComponentLocation();
    const FVector ShotDirection = FollowCamera->GetForwardVector();
    const FVector TraceEnd = TraceStart + ShotDirection * Data->FireRange;
    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(RazigraGun), true, this);
    Params.AddIgnoredActor(this);
    Params.AddIgnoredComponent(GetCapsuleComponent());
    Params.AddIgnoredComponent(GetMesh());
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
    const FName OriginBone = UGlobalGameData::Get(this)->VisualTraceOriginBoneName;
    if (const USkeletalMeshComponent* MeshComponent = GetMesh())
    {
        if (!OriginBone.IsNone() && MeshComponent->DoesSocketExist(OriginBone))
        {
            return MeshComponent->GetSocketLocation(OriginBone);
        }
    }
    UE_LOG(LogRazigra, Warning, TEXT("Visual trace origin bone/socket '%s' was not found; using camera origin."),
        *OriginBone.ToString());
    return FollowCamera ? FollowCamera->GetComponentLocation() : GetActorLocation();
}

void ASharedHeroCharacter::MulticastGunFired_Implementation(const FVector_NetQuantize& MuzzleLocation,
    const FVector_NetQuantize& ImpactPoint, bool bHit, AActor* HitActor)
{
    BP_OnGunFired(MuzzleLocation, ImpactPoint, bHit, HitActor);
    FActorSpawnParameters TraceParams;
    TraceParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    TraceParams.ObjectFlags |= RF_Transient;
    if (ALaserTraceActor* Laser = GetWorld()->SpawnActor<ALaserTraceActor>(
        ALaserTraceActor::StaticClass(), FVector(MuzzleLocation), FRotator::ZeroRotator, TraceParams))
    {
        Laser->InitializeLaser(MuzzleLocation, ImpactPoint);
    }
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
    if (!HasAuthority() || bIsDead || bShieldActive)
    {
        return 0.0f;
    }
    const float Applied = FMath::Min(Health, FMath::Max(0.0f, DamageAmount));
    Health -= Applied;
    UE_LOG(LogRazigra, Log, TEXT("Hero received %.1f damage from %s (health %.1f/%.1f)."), Applied,
        DamageCauser ? *DamageCauser->GetName() : TEXT("unknown"), Health,
        UGlobalGameData::Get(this)->HeroMaxHealth);
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

float ASharedHeroCharacter::GetGroundSpeed() const
{
    return GetVelocity().Size2D();
}

float ASharedHeroCharacter::GetMovementDirection() const
{
    const FVector Travel = GetVelocity();
    if (Travel.IsNearlyZero())
    {
        return 0.0f;
    }
    const FMatrix Basis = FRotationMatrix(GetActorRotation());
    const FVector Heading = Travel.GetSafeNormal2D();
    const float Angle = FMath::RadiansToDegrees(FMath::Acos(
        FMath::Clamp(static_cast<float>(FVector::DotProduct(Basis.GetScaledAxis(EAxis::X), Heading)), -1.0f, 1.0f)));
    return FVector::DotProduct(Basis.GetScaledAxis(EAxis::Y), Heading) < 0.0 ? -Angle : Angle;
}

bool ASharedHeroCharacter::ShouldMove() const
{
    return !bIsDead && GetGroundSpeed() > 3.0f;
}

float ASharedHeroCharacter::GetAimPitch() const
{
    return FRotator::NormalizeAxis(AimRotation.Pitch);
}

float ASharedHeroCharacter::GetAimYaw() const
{
    return FRotator::NormalizeAxis(AimRotation.Yaw);
}

FRotator ASharedHeroCharacter::GetBaseAimRotation() const
{
    return FRotator(FRotator::NormalizeAxis(AimRotation.Pitch), FRotator::NormalizeAxis(AimRotation.Yaw), 0.0f);
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
    DOREPLIFETIME(ASharedHeroCharacter, bHealingActive);
    DOREPLIFETIME(ASharedHeroCharacter, bShieldActive);
    DOREPLIFETIME(ASharedHeroCharacter, RecoilHeat);
    DOREPLIFETIME(ASharedHeroCharacter, bWeaponOverheated);
    DOREPLIFETIME(ASharedHeroCharacter, RequiredConsensusParticipants);
    DOREPLIFETIME(ASharedHeroCharacter, PlayerOneActionMask);
    DOREPLIFETIME(ASharedHeroCharacter, PlayerTwoActionMask);
    DOREPLIFETIME(ASharedHeroCharacter, PlayerOneLookAxis);
    DOREPLIFETIME(ASharedHeroCharacter, PlayerTwoLookAxis);
}
