#include "ZombieCharacter.h"

#include "AIController.h"
#include "CoopGameState.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GlobalGameData.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "SharedHeroCharacter.h"

AZombieCharacter::AZombieCharacter()
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = true;
    SetReplicateMovement(true);
    SetNetUpdateFrequency(30.0f);
    AIControllerClass = AAIController::StaticClass();
    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
    GetCharacterMovement()->bOrientRotationToMovement = true;
    GetCharacterMovement()->RotationRate = FRotator(0.0f, 420.0f, 0.0f);
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
    const bool bInAttackRange = DistanceSquared <= FMath::Square(Data->ZombieAttackRange);
    AAIController* AI = Cast<AAIController>(GetController());

    if (bInAttackRange)
    {
        if (AI)
        {
            AI->StopMovement();
        }
        SetActorRotation((TargetHero->GetActorLocation() - GetActorLocation()).Rotation());
        if (Now >= NextAttackTime)
        {
            NextAttackTime = Now + Data->ZombieAttackInterval;
            AttackVisualUntil = Now + FMath::Min(Data->ZombieAttackInterval, 0.35f);
            bIsAttacking = true;
            UGameplayStatics::ApplyDamage(TargetHero, Data->ZombieAttackDamage, AI, this, UDamageType::StaticClass());
            MulticastAttack();
        }
    }
    else if (AI && Now >= NextPathRefreshTime)
    {
        NextPathRefreshTime = Now + Data->ZombiePathRefreshInterval;
        AI->MoveToActor(TargetHero, Data->ZombieAttackRange * 0.8f, true, true, true, nullptr, true);
    }

    if (bIsAttacking && Now >= AttackVisualUntil)
    {
        bIsAttacking = false;
    }
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
    if (Health <= 0.0f)
    {
        bIsDead = true;
        bIsAttacking = false;
        GetCharacterMovement()->DisableMovement();
        SetActorEnableCollision(false);
        MulticastDied();
        SetLifeSpan(3.0f);
    }
    return Applied;
}

void AZombieCharacter::MulticastAttack_Implementation()
{
    BP_OnZombieAttack();
}

void AZombieCharacter::MulticastDied_Implementation()
{
    BP_OnZombieDied();
}

void AZombieCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AZombieCharacter, Health);
    DOREPLIFETIME(AZombieCharacter, bIsAttacking);
    DOREPLIFETIME(AZombieCharacter, bIsDead);
}
