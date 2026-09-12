#include "LaserTraceActor.h"

#include "Components/StaticMeshComponent.h"
#include "GlobalGameData.h"
#include "GamejamRazigra.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

ALaserTraceActor::ALaserTraceActor()
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = false;
    SetActorEnableCollision(false);
    LaserMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LaserMesh"));
    SetRootComponent(LaserMesh);
    LaserMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    LaserMesh->SetCastShadow(false);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    if (Cylinder.Succeeded())
    {
        LaserMesh->SetStaticMesh(Cylinder.Object);
    }
}

void ALaserTraceActor::InitializeLaser(const FVector& Start, const FVector& End)
{
    const FVector Delta = End - Start;
    const float Length = Delta.Size();
    if (Length <= KINDA_SMALL_NUMBER)
    {
        Destroy();
        return;
    }

    const UGlobalGameData* Data = UGlobalGameData::Get(this);
    Lifetime = FMath::Max(0.01f, Data->LaserTraceLifetime);
    SetLifeSpan(Lifetime);
    SetActorLocation((Start + End) * 0.5f);
    SetActorRotation(FRotationMatrix::MakeFromZ(Delta / Length).Rotator());
    const float RadiusScale = Data->LaserTraceThickness / 50.0f;
    LaserMesh->SetRelativeScale3D(FVector(RadiusScale, RadiusScale, Length / 100.0f));

    UMaterialInterface* BaseMaterial = Data->LaserTraceMaterial.LoadSynchronous();
    const UMaterialInterface* EngineDefault = UMaterial::GetDefaultMaterial(MD_Surface);
    if (!BaseMaterial || BaseMaterial == EngineDefault)
    {
        if (BaseMaterial == EngineDefault)
        {
            UE_LOG(LogRazigra, Warning,
                TEXT("GlobalGameData laser material resolved to the engine default; forcing M_LaserTrace."));
        }
        BaseMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materials/M_LaserTrace.M_LaserTrace"));
    }
    if (BaseMaterial)
    {
        LaserMaterial = UMaterialInstanceDynamic::Create(BaseMaterial, this);
        LaserMaterial->SetVectorParameterValue(TEXT("LaserColor"), Data->LaserTraceColor);
        LaserMaterial->SetScalarParameterValue(TEXT("Intensity"), Data->LaserTraceIntensity);
        LaserMaterial->SetScalarParameterValue(TEXT("Opacity"), 1.0f);
        LaserMesh->SetMaterial(0, LaserMaterial);
        UE_LOG(LogRazigra, Log, TEXT("Laser trace using material %s (MID %s)."),
            *BaseMaterial->GetPathName(), *LaserMaterial->GetPathName());
    }
    else
    {
        UE_LOG(LogRazigra, Error, TEXT("Laser trace material failed to load; no material was assigned."));
    }
}

void ALaserTraceActor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    Elapsed += DeltaSeconds;
    if (LaserMaterial)
    {
        LaserMaterial->SetScalarParameterValue(TEXT("Opacity"),
            FMath::Square(1.0f - FMath::Clamp(Elapsed / Lifetime, 0.0f, 1.0f)));
    }
}
