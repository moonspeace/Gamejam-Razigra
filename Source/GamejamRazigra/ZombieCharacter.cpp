#include "ZombieCharacter.h"

#include "AIController.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "CoopGameState.h"
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
#include "SharedHeroCharacter.h"

AZombieCharacter::AZombieCharacter()
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = true;
    SetReplicateMovement(true);
    SetNetUpdateFrequency(30.0f);
    SetCanBeDamaged(true);
    AIControllerClass = AAIController::StaticClass();
    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
    GetCharacterMovement()->bOrientRotationToMovement = true;
    GetCharacterMovement()->RotationRate = FRotator(0.0f, 420.0f, 0.0f);
    // Camera weapon traces use Visibility. Pawn collision ignores it by default.
    GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
}

void AZombieCharacter::BeginPlay()
{
    Super::BeginPlay();
    const UGlobalGameData* Data = UGlobalGameData::Get(this);
    Health = Data->ZombieMaxHealth;
    GetCharacterMovement()->MaxWalkSpeed = Data->ZombieMoveSpeed;

    if (USkeletalMesh* MeshAsset = Data->ZombieMesh.LoadSynchronous())
    {
        GetMesh()->SetSkeletalMeshAsset(MeshAsset);
        GetMesh()->SetRelativeLocationAndRotation(FVector(0.0f, 0.0f, -96.0f), FRotator(0.0f, -90.0f, 0.0f));
    }
    if (UClass* AnimationClass = Data->ZombieAnimationClass.LoadSynchronous())
    {
        GetMesh()->SetAnimInstanceClass(AnimationClass);
    }
}

void AZombieCharacter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (bIsDead)
    {
        UpdateDeathEffect(DeltaSeconds);
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
        if (IsValid(TargetHero))
        {
            SetActorRotation((TargetHero->GetActorLocation() - GetActorLocation()).Rotation());
        }
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
        SetActorRotation((TargetHero->GetActorLocation() - GetActorLocation()).Rotation());
        if (Now >= NextAttackTime)
        {
            StartAttack();
        }
    }
    else if (AI && Now >= NextPathRefreshTime)
    {
        NextPathRefreshTime = Now + Data->ZombiePathRefreshInterval;
        AI->MoveToActor(TargetHero, MeleeReach * 0.8f, true, true, true, nullptr, true);
    }

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
    const bool bHitHero = IsValid(TargetHero) && !TargetHero->IsDead() && Distance <= MeleeReach;
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
    if (!HasAuthority() || bIsDead)
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
        MulticastDamageReceived(Applied);
    }
    if (Health <= 0.0f)
    {
        if (ACoopGameState* State = GetWorld()->GetGameState<ACoopGameState>())
        {
            State->AddZombieKill();
        }
        bIsDead = true;
        bIsAttacking = false;
        GetCharacterMovement()->DisableMovement();
        SetActorEnableCollision(false);
        MulticastDied();
        const UGlobalGameData* Data = UGlobalGameData::Get(this);
        SetLifeSpan(Data->ZombieDeathBlinkDuration + Data->ZombieDeathDissolveDuration + 0.15f);
    }
    return Applied;
}

void AZombieCharacter::MulticastDamageReceived_Implementation(float DamageAmount)
{
    BP_OnZombieDamaged(DamageAmount);
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

void AZombieCharacter::MulticastAttack_Implementation()
{
    const UGlobalGameData* Data = UGlobalGameData::Get(this);
    if (UAnimMontage* AttackMontage = Data->ZombieAttackMontage.LoadSynchronous())
    {
        if (UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
        {
            AnimInstance->Montage_Play(AttackMontage);
        }
    }
    BP_OnZombieAttack();
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
