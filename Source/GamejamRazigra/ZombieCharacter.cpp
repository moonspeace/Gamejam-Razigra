#include "ZombieCharacter.h"

#include "AIController.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequenceBase.h"
#include "CoopGameState.h"
#include "Components/CapsuleComponent.h"
#include "DamageNumberActor.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GamejamRazigra.h"
#include "GlobalGameData.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "NavigationSystem.h"
#include "SharedHeroCharacter.h"
#include "TimerManager.h"
#include "EngineUtils.h"

AZombieCharacter::AZombieCharacter()
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = true;
    SetReplicateMovement(true);
    SetNetUpdateFrequency(30.0f);
    SetCanBeDamaged(true);
    AIControllerClass = AAIController::StaticClass();
    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
    // APawn defaults bUseControllerRotationYaw to true, which on the server hands the zombie's
    // yaw to its AIController. Clients have no controller, so they fall back to the replicated
    // rotation and end up facing somewhere else entirely. Turning it off leaves exactly one
    // system in charge of facing on both sides.
    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = false;
    bUseControllerRotationRoll = false;
    GetCharacterMovement()->bOrientRotationToMovement = true;
    GetCharacterMovement()->bUseControllerDesiredRotation = false;
    GetCharacterMovement()->RotationRate = FRotator(0.0f, 420.0f, 0.0f);
    GetCharacterMovement()->bUseRVOAvoidance = true;
    GetCharacterMovement()->AvoidanceConsiderationRadius = 420.0f;
    GetCharacterMovement()->AvoidanceWeight = 0.65f;
    // Camera weapon traces use Visibility. Pawn collision ignores it by default.
    GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    // A crowd behind the hero must never push the third-person camera into the character.
    // Camera booms probe on ECC_Camera, independently from weapon visibility traces.
    GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);

    // These two must be set here and not in BeginPlay. ACharacter::PostInitializeComponents caches
    // the mesh's relative transform into BaseTranslationOffset/BaseRotationOffset, and that runs
    // BEFORE BeginPlay. On clients the movement component's network smoothing rebuilds the mesh
    // transform from those cached values every frame, so an offset applied later is thrown away:
    // the mesh snaps back to the capsule centre and un-rotated, which is the zombie floating a
    // capsule's height off the ground and facing ninety degrees off. The server never smooths, so
    // it looked correct there.
    GetCapsuleComponent()->InitCapsuleSize(42.0f, 96.0f);
    GetMesh()->SetRelativeLocationAndRotation(FVector(0.0f, 0.0f, -96.0f), FRotator(0.0f, -90.0f, 0.0f));
}

void AZombieCharacter::BeginPlay()
{
    Super::BeginPlay();
    const UGlobalGameData* Data = UGlobalGameData::Get(this);
    Health = Data->ZombieMaxHealth;
    GetCharacterMovement()->MaxWalkSpeed = Data->ZombieMoveSpeed;
    // Stable per-agent formation phase prevents every zombie choosing the same approach lane.
    TacticalAngleRadians = FMath::Fmod(static_cast<float>(GetUniqueID()) * 2.39996323f, 2.0f * PI);
    LastProgressLocation = GetActorLocation();
    LastProgressCheckTime = GetWorld()->GetTimeSeconds();

    if (USkeletalMesh* MeshAsset = Data->ZombieMesh.LoadSynchronous())
    {
        // Mesh asset only: its relative transform belongs in the constructor, see the note there.
        GetMesh()->SetSkeletalMeshAsset(MeshAsset);
    }
    if (UClass* AnimationClass = Data->ZombieAnimationClass.LoadSynchronous())
    {
        GetMesh()->SetAnimInstanceClass(AnimationClass);
    }
    StartSpawnEffect();
}

void AZombieCharacter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (bIsDead)
    {
        UpdateDeathEffect(DeltaSeconds);
        return;
    }
    if (bSpawnEffectActive)
    {
        UpdateSpawnEffect(DeltaSeconds);
        return;
    }
    if (HasAuthority() && !bIsDead)
    {
        UpdateServerBehavior();
    }
}

void AZombieCharacter::AcquireTarget()
{
    if (const ACoopGameState* State = GetWorld()->GetGameState<ACoopGameState>())
    {
        TargetHero = State->SharedHero;
    }
}

/**
 * Turns to the hero on the yaw axis only. The old version fed the full 3D delta to SetActorRotation,
 * which pitched and rolled the capsule whenever the hero was above or below, and fought
 * bOrientRotationToMovement for the same rotation in the same frame. Disabling orient-to-movement
 * first means the server settles on one answer, which is then the one that replicates.
 */
void AZombieCharacter::FaceTarget()
{
    if (!IsValid(TargetHero))
    {
        return;
    }
    GetCharacterMovement()->bOrientRotationToMovement = false;
    const FVector ToTarget = TargetHero->GetActorLocation() - GetActorLocation();
    SetActorRotation(FRotator(0.0f, ToTarget.Rotation().Yaw, 0.0f));
}

void AZombieCharacter::UpdateServerBehavior()
{
    if (!IsValid(TargetHero) || TargetHero->IsDead())
    {
        AcquireTarget();
    }
    if (!IsValid(TargetHero) || TargetHero->IsDead())
    {
        return;
    }

    const UGlobalGameData* Data = UGlobalGameData::Get(this);
    const double Now = GetWorld()->GetTimeSeconds();
    const float DistanceSquared = FVector::DistSquared2D(GetActorLocation(), TargetHero->GetActorLocation());
    const float TargetRadius = TargetHero->GetCapsuleComponent()->GetScaledCapsuleRadius();
    const float ZombieRadius = GetCapsuleComponent()->GetScaledCapsuleRadius();
    const float MeleeReach = Data->ZombieAttackRange + TargetRadius + ZombieRadius;
    const bool bInAttackRange = DistanceSquared <= FMath::Square(MeleeReach);
    AAIController* AI = Cast<AAIController>(GetController());

    if (bIsAttacking)
    {
        if (AI)
        {
            AI->StopMovement();
        }
        FaceTarget();
        if (Now >= AttackHitTime)
        {
            ResolveAttack();
        }
        return;
    }

    if (bInAttackRange)
    {
        if (AI)
        {
            AI->StopMovement();
        }
        FaceTarget();
        if (Now >= NextAttackTime)
        {
            StartAttack();
        }
    }
    else if (AI)
    {
        const FVector HeroLocation = TargetHero->GetActorLocation();
        const bool bHeroMoved = FVector::DistSquared2D(HeroLocation, LastTrackedHeroLocation)
            >= FMath::Square(Data->ZombieReactiveRepathDistance);
        bool bRecovering = false;
        if (Now - LastProgressCheckTime >= Data->ZombieStuckRecoverySeconds)
        {
            const float Progress = FVector::Dist2D(GetActorLocation(), LastProgressLocation);
            bRecovering = Progress < 18.0f && GetVelocity().Size2D() < 35.0f;
            LastProgressLocation = GetActorLocation();
            LastProgressCheckTime = Now;
        }
        if (Now >= NextPathRefreshTime || bHeroMoved || bRecovering)
        {
            GetCharacterMovement()->bOrientRotationToMovement = true;
            // Never let an old serialized data asset make reactions feel sluggish.
            NextPathRefreshTime = Now + FMath::Min(Data->ZombiePathRefreshInterval, 0.18f);
            LastTrackedHeroLocation = HeroLocation;
            const FVector Goal = CalculateTacticalGoal(bRecovering);
            AI->MoveToLocation(Goal, 28.0f, true, true, true, false, nullptr, true);
        }
    }

}

FVector AZombieCharacter::CalculateTacticalGoal(bool bRecovering) const
{
    const UGlobalGameData* Data = UGlobalGameData::Get(this);
    FVector PredictedHero = TargetHero->GetActorLocation()
        + TargetHero->GetVelocity() * Data->ZombieTargetPredictionSeconds;

    // Slowly orbit each stable slot, producing flanks without making the formation spin wildly.
    const float Time = GetWorld()->GetTimeSeconds();
    const float Angle = TacticalAngleRadians + Time * (bRecovering ? 0.9f : 0.10f);
    const FVector RingOffset(FMath::Cos(Angle), FMath::Sin(Angle), 0.0f);
    FVector Goal = PredictedHero + RingOffset * Data->ZombieEngagementRadius;

    FVector Separation = FVector::ZeroVector;
    int32 NeighborCount = 0;
    for (TActorIterator<AZombieCharacter> It(GetWorld()); It; ++It)
    {
        const AZombieCharacter* Other = *It;
        if (Other == this || Other->IsDead()) continue;
        FVector Away = GetActorLocation() - Other->GetActorLocation();
        Away.Z = 0.0f;
        const float Distance = Away.Size();
        if (Distance > KINDA_SMALL_NUMBER && Distance < Data->ZombieSeparationRadius)
        {
            Separation += Away / Distance * (1.0f - Distance / Data->ZombieSeparationRadius);
            ++NeighborCount;
        }
    }
    if (NeighborCount > 0)
    {
        Goal += Separation.GetClampedToMaxSize(1.0f)
            * Data->ZombieSeparationRadius * Data->ZombieSeparationStrength;
    }
    if (bRecovering)
    {
        // A decisive tangent step breaks capsule queues and asks navigation for a new corridor.
        Goal += FVector(-RingOffset.Y, RingOffset.X, 0.0f) * Data->ZombieSeparationRadius * 1.5f;
    }

    FNavLocation Projected;
    if (UNavigationSystemV1* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
    {
        if (Nav->ProjectPointToNavigation(Goal, Projected, FVector(180.0f, 180.0f, 260.0f)))
        {
            return Projected.Location;
        }
    }
    return Goal;
}

void AZombieCharacter::StartAttack()
{
    const UGlobalGameData* Data = UGlobalGameData::Get(this);
    bIsAttacking = true;
    AttackHitTime = GetWorld()->GetTimeSeconds() + Data->ZombieAttackWindupSeconds;
    UE_LOG(LogRazigra, Log, TEXT("Zombie %s started an attack (damage %.1f, windup %.2fs)."),
        *GetName(), Data->ZombieAttackDamage, Data->ZombieAttackWindupSeconds);
    MulticastAttack();
}

void AZombieCharacter::ResolveAttack()
{
    const UGlobalGameData* Data = UGlobalGameData::Get(this);
    const float TargetRadius = IsValid(TargetHero) ? TargetHero->GetCapsuleComponent()->GetScaledCapsuleRadius() : 0.0f;
    const float ZombieRadius = GetCapsuleComponent()->GetScaledCapsuleRadius();
    const float MeleeReach = Data->ZombieAttackRange + TargetRadius + ZombieRadius;
    const float Distance = IsValid(TargetHero)
        ? FVector::Dist2D(GetActorLocation(), TargetHero->GetActorLocation())
        : TNumericLimits<float>::Max();
    bool bHitHero = false;
    FHitResult ObstructionHit;
    if (IsValid(TargetHero) && !TargetHero->IsDead()
        && !TargetHero->IsCrouchDamageImmunityActive() && Distance <= MeleeReach)
    {
        const FVector Start = GetActorLocation() + FVector::UpVector * 65.0f;
        const FVector End = TargetHero->GetActorLocation() + FVector::UpVector * 65.0f;
        FCollisionQueryParams TraceParams(SCENE_QUERY_STAT(ZombieMeleeVisibility), false, this);
        TraceParams.AddIgnoredActor(this);
        const bool bSweepHit = GetWorld()->SweepSingleByChannel(ObstructionHit, Start, End,
            FQuat::Identity, ECC_Visibility,
            FCollisionShape::MakeSphere(Data->ZombieAttackTraceRadius), TraceParams);
        bHitHero = bSweepHit && ObstructionHit.GetActor() == TargetHero;
        if (bSweepHit && !bHitHero)
        {
            UE_LOG(LogRazigra, Verbose, TEXT("Zombie %s melee blocked by %s."), *GetName(),
                ObstructionHit.GetActor() ? *ObstructionHit.GetActor()->GetName() : TEXT("world geometry"));
        }
    }
    if (bHitHero)
    {
        const float Applied = UGameplayStatics::ApplyDamage(TargetHero, Data->ZombieAttackDamage,
            Cast<AAIController>(GetController()), this, UDamageType::StaticClass());
        UE_LOG(LogRazigra, Log, TEXT("Zombie %s attack hit at %.1f/%.1f units; applied %.1f damage."),
            *GetName(), Distance, MeleeReach, Applied);
    }
    else
    {
        UE_LOG(LogRazigra, Log, TEXT("Zombie %s attack missed at %.1f/%.1f units."),
            *GetName(), Distance, MeleeReach);
    }
    bIsAttacking = false;
    NextAttackTime = GetWorld()->GetTimeSeconds() + Data->ZombieAttackInterval;
    MulticastAttackResolved(bHitHero);
}

float AZombieCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent,
    AController* EventInstigator, AActor* DamageCauser)
{
    if (!HasAuthority() || bIsDead || bSpawnEffectActive)
    {
        return 0.0f;
    }
    const float Applied = FMath::Min(Health, FMath::Max(0.0f, DamageAmount));
    Health -= Applied;
    UE_LOG(LogRazigra, Log, TEXT("Zombie %s received %.1f damage from %s (health %.1f/%.1f)."),
        *GetName(), Applied, DamageCauser ? *DamageCauser->GetName() : TEXT("unknown"), Health,
        UGlobalGameData::Get(this)->ZombieMaxHealth);
    if (Applied > 0.0f)
    {
        MulticastDamageReceived(Applied, Health <= 0.0f);
    }
    if (Health <= 0.0f)
    {
        if (ACoopGameState* State = GetWorld()->GetGameState<ACoopGameState>())
        {
            State->AddZombieKill();
        }
        bIsDead = true;
        bIsAttacking = false;
        GetWorldTimerManager().ClearTimer(RestoreAnimationTimer);
        GetWorldTimerManager().ClearTimer(HitFlashTimer);
        PreHitMaterials.Reset();
        GetCharacterMovement()->DisableMovement();
        GetCharacterMovement()->StopMovementImmediately();
        if (AAIController* AI = Cast<AAIController>(GetController()))
        {
            AI->StopMovement();
        }
        if (GetMesh())
        {
            GetMesh()->bPauseAnims = true;
            GetMesh()->SetComponentTickEnabled(false);
        }
        SetActorEnableCollision(false);
        MulticastDied();
        const UGlobalGameData* Data = UGlobalGameData::Get(this);
        SetLifeSpan(Data->ZombieDeathBlinkDuration + Data->ZombieDeathDissolveDuration + 0.15f);
    }
    return Applied;
}

void AZombieCharacter::MulticastDamageReceived_Implementation(float DamageAmount, bool bLethalHit)
{
    BP_OnZombieDamaged(DamageAmount);
    if (!bLethalHit)
    {
        StartHitEffect();
    }
    if (!GetWorld())
    {
        return;
    }

    APlayerController* LocalController = nullptr;
    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        if (APlayerController* CandidateController = It->Get())
        {
            if (CandidateController->IsLocalController())
            {
                LocalController = CandidateController;
                break;
            }
        }
    }
    if (!LocalController)
    {
        return;
    }

    const UGlobalGameData* Data = UGlobalGameData::Get(this);
    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    Params.ObjectFlags |= RF_Transient;
    const FBox Bounds = GetComponentsBoundingBox(true);
    const FVector NumberLocation(GetActorLocation().X, GetActorLocation().Y,
        Bounds.IsValid ? Bounds.Max.Z + FMath::Max(30.0f, Data->DamageNumberHeight * 0.25f)
                       : GetActorLocation().Z + Data->DamageNumberHeight);
    if (ADamageNumberActor* Number = GetWorld()->SpawnActor<ADamageNumberActor>(
        ADamageNumberActor::StaticClass(), NumberLocation, FRotator::ZeroRotator, Params))
    {
        Number->InitializeDamageNumber(DamageAmount, LocalController);
    }
}

void AZombieCharacter::StartSpawnEffect()
{
    if (!GetMesh()) return;
    const UGlobalGameData* Data = UGlobalGameData::Get(this);
    UMaterialInterface* SpawnMaterial = Data->ZombieSpawnMaterial.LoadSynchronous();
    if (!SpawnMaterial)
    {
        // Old GlobalGameData assets serialize newly-added soft references as None, so retain a
        // cooked fallback until the asset is opened and resaved by a designer.
        SpawnMaterial = LoadObject<UMaterialInterface>(nullptr,
            TEXT("/Game/Materials/M_ZombieDeath.M_ZombieDeath"));
    }
    if (!SpawnMaterial)
    {
        UE_LOG(LogRazigra, Warning, TEXT("Zombie spawn material is missing for %s."), *GetName());
        return;
    }

    SpawnEffectElapsed = 0.0f;
    bSpawnEffectActive = true;
    SpawnMaterials.Reset();
    PreSpawnMaterials.Reset();
    const int32 SlotCount = FMath::Max(1, GetMesh()->GetNumMaterials());
    for (int32 Slot = 0; Slot < SlotCount; ++Slot)
    {
        PreSpawnMaterials.Add(GetMesh()->GetMaterial(Slot));
        UMaterialInstanceDynamic* Dynamic = UMaterialInstanceDynamic::Create(SpawnMaterial, this);
        Dynamic->SetVectorParameterValue(TEXT("DeathColor"),
            Data->ZombieSpawnEffectColor * Data->ZombieSpawnEffectIntensity);
        Dynamic->SetVectorParameterValue(TEXT("SpawnColor"), Data->ZombieSpawnEffectColor);
        Dynamic->SetScalarParameterValue(TEXT("DissolveAmount"), 1.0f);
        Dynamic->SetScalarParameterValue(TEXT("BlinkAmount"), 1.0f);
        Dynamic->SetScalarParameterValue(TEXT("EdgeWidth"), Data->ZombieSpawnEdgeWidth);
        GetMesh()->SetMaterial(Slot, Dynamic);
        SpawnMaterials.Add(Dynamic);
    }
    UE_LOG(LogRazigra, Verbose, TEXT("Zombie %s assembling with spawn material %s."),
        *GetName(), *SpawnMaterial->GetPathName());
}

void AZombieCharacter::UpdateSpawnEffect(float DeltaSeconds)
{
    const UGlobalGameData* Data = UGlobalGameData::Get(this);
    SpawnEffectElapsed += DeltaSeconds;
    const float Alpha = FMath::Clamp(SpawnEffectElapsed /
        FMath::Max(0.05f, Data->ZombieSpawnEffectDuration), 0.0f, 1.0f);
    const float Dissolve = 1.0f - FMath::InterpEaseOut(0.0f, 1.0f, Alpha, 2.2f);
    const float Pulse = FMath::Clamp((1.0f - Alpha)
        * (0.72f + 0.28f * FMath::Sin(Alpha * 8.0f * PI)), 0.0f, 1.0f);
    for (UMaterialInstanceDynamic* Material : SpawnMaterials)
    {
        if (!Material) continue;
        Material->SetScalarParameterValue(TEXT("DissolveAmount"), Dissolve);
        Material->SetScalarParameterValue(TEXT("BlinkAmount"), Pulse);
    }
    if (Alpha >= 1.0f) FinishSpawnEffect();
}

void AZombieCharacter::FinishSpawnEffect()
{
    if (GetMesh())
    {
        for (int32 Slot = 0; Slot < PreSpawnMaterials.Num(); ++Slot)
        {
            GetMesh()->SetMaterial(Slot, PreSpawnMaterials[Slot]);
        }
    }
    SpawnMaterials.Reset();
    PreSpawnMaterials.Reset();
    bSpawnEffectActive = false;
}

void AZombieCharacter::StartHitEffect()
{
    if (!GetMesh() || bIsDead) return;
    const UGlobalGameData* Data = UGlobalGameData::Get(this);
    UMaterialInterface* FlashBase = Data->ZombieDeathMaterial.LoadSynchronous();
    if (!FlashBase)
    {
        FlashBase = LoadObject<UMaterialInterface>(nullptr,
            TEXT("/Game/Materials/M_ZombieDeath.M_ZombieDeath"));
    }
    if (!FlashBase) return;

    const int32 SlotCount = FMath::Max(1, GetMesh()->GetNumMaterials());
    if (PreHitMaterials.IsEmpty())
    {
        PreHitMaterials.Reserve(SlotCount);
        for (int32 Slot = 0; Slot < SlotCount; ++Slot)
        {
            PreHitMaterials.Add(GetMesh()->GetMaterial(Slot));
        }
    }
    for (int32 Slot = 0; Slot < SlotCount; ++Slot)
    {
        UMaterialInstanceDynamic* Flash = UMaterialInstanceDynamic::Create(FlashBase, this);
        Flash->SetVectorParameterValue(TEXT("DeathColor"),
            Data->ZombieHitFlashColor * Data->ZombieHitFlashIntensity);
        Flash->SetScalarParameterValue(TEXT("BlinkAmount"), 1.0f);
        Flash->SetScalarParameterValue(TEXT("DissolveAmount"), 0.0f);
        GetMesh()->SetMaterial(Slot, Flash);
    }
    GetWorldTimerManager().SetTimer(HitFlashTimer, this, &ThisClass::RestoreHitEffect,
        Data->ZombieHitFlashDuration, false);
}

void AZombieCharacter::RestoreHitEffect()
{
    if (!GetMesh() || bIsDead) { PreHitMaterials.Reset(); return; }
    for (int32 Slot = 0; Slot < PreHitMaterials.Num(); ++Slot)
    {
        GetMesh()->SetMaterial(Slot, PreHitMaterials[Slot]);
    }
    PreHitMaterials.Reset();
}

void AZombieCharacter::MulticastAttack_Implementation()
{
    const UGlobalGameData* Data = UGlobalGameData::Get(this);
    UAnimSequenceBase* DirectAttack = Data->ZombieAttackAnimation.LoadSynchronous();
    if (DirectAttack && GetMesh())
    {
        GetMesh()->PlayAnimation(DirectAttack, false);
        GetWorldTimerManager().SetTimer(RestoreAnimationTimer, this,
            &ThisClass::RestoreAnimationBlueprint, FMath::Max(0.05f, DirectAttack->GetPlayLength()), false);
    }
    else if (UAnimMontage* AttackMontage = Data->ZombieAttackMontage.LoadSynchronous())
    {
        GetMesh()->PlayAnimation(AttackMontage, false);
        GetWorldTimerManager().SetTimer(RestoreAnimationTimer, this,
            &ThisClass::RestoreAnimationBlueprint, FMath::Max(0.05f, AttackMontage->GetPlayLength()), false);
    }
    BP_OnZombieAttack();
}

void AZombieCharacter::RestoreAnimationBlueprint()
{
    if (!GetMesh() || bIsDead)
    {
        return;
    }
    if (UClass* AnimationClass = UGlobalGameData::Get(this)->ZombieAnimationClass.LoadSynchronous())
    {
        GetMesh()->SetAnimationMode(EAnimationMode::AnimationBlueprint);
        GetMesh()->SetAnimInstanceClass(AnimationClass);
    }
}

void AZombieCharacter::MulticastDied_Implementation()
{
    StartDeathEffect();
    BP_OnZombieDied();
}

void AZombieCharacter::StartDeathEffect()
{
    DeathEffectElapsed = 0.0f;
    DeathMaterials.Reset();
    const UGlobalGameData* Data = UGlobalGameData::Get(this);
    UMaterialInterface* DeathMaterial = Data->ZombieDeathMaterial.LoadSynchronous();
    if (!DeathMaterial)
    {
        // Existing GlobalGameData assets can have serialized the newly-added soft reference as
        // None. Keep the effect operational even before a designer resaves that data asset.
        DeathMaterial = LoadObject<UMaterialInterface>(nullptr,
            TEXT("/Game/Materials/M_ZombieDeath.M_ZombieDeath"));
    }
    if (!DeathMaterial || !GetMesh())
    {
        UE_LOG(LogRazigra, Warning, TEXT("Zombie death material is missing; %s will use visibility blinking."), *GetName());
        return;
    }
    UE_LOG(LogRazigra, Log, TEXT("Zombie %s starting death effect with material %s."),
        *GetName(), *DeathMaterial->GetPathName());

    const int32 SlotCount = FMath::Max(1, GetMesh()->GetNumMaterials());
    for (int32 Slot = 0; Slot < SlotCount; ++Slot)
    {
        UMaterialInstanceDynamic* DynamicMaterial = UMaterialInstanceDynamic::Create(DeathMaterial, this);
        DynamicMaterial->SetVectorParameterValue(TEXT("DeathColor"), FLinearColor(1.0f, 0.0f, 0.0f, 1.0f));
        DynamicMaterial->SetScalarParameterValue(TEXT("BlinkAmount"), 0.0f);
        DynamicMaterial->SetScalarParameterValue(TEXT("DissolveAmount"), 0.0f);
        GetMesh()->SetMaterial(Slot, DynamicMaterial);
        DeathMaterials.Add(DynamicMaterial);
    }
}

void AZombieCharacter::UpdateDeathEffect(float DeltaSeconds)
{
    const UGlobalGameData* Data = UGlobalGameData::Get(this);
    DeathEffectElapsed += DeltaSeconds;
    const float BlinkDuration = Data->ZombieDeathBlinkDuration;
    const bool bBlinking = DeathEffectElapsed < BlinkDuration;
    const float Blink = bBlinking
        ? (FMath::Sin(DeathEffectElapsed * Data->ZombieDeathBlinkFrequency * 2.0f * PI) >= 0.0f ? 1.0f : 0.05f)
        : 1.0f;
    const float Dissolve = bBlinking ? 0.0f : FMath::Clamp(
        (DeathEffectElapsed - BlinkDuration) / FMath::Max(0.05f, Data->ZombieDeathDissolveDuration), 0.0f, 1.0f);

    if (DeathMaterials.IsEmpty())
    {
        if (GetMesh())
        {
            GetMesh()->SetVisibility(!bBlinking || Blink > 0.5f, true);
            if (!bBlinking)
            {
                GetMesh()->SetWorldScale3D(FVector(FMath::Max(0.01f, 1.0f - Dissolve)));
            }
        }
        return;
    }

    for (UMaterialInstanceDynamic* Material : DeathMaterials)
    {
        if (Material)
        {
            Material->SetScalarParameterValue(TEXT("BlinkAmount"), Blink);
            Material->SetScalarParameterValue(TEXT("DissolveAmount"), Dissolve);
        }
    }
}

void AZombieCharacter::MulticastAttackResolved_Implementation(bool bHitHero)
{
    BP_OnZombieAttackResolved(bHitHero);
}

void AZombieCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AZombieCharacter, Health);
    DOREPLIFETIME(AZombieCharacter, bIsAttacking);
    DOREPLIFETIME(AZombieCharacter, bIsDead);
}
