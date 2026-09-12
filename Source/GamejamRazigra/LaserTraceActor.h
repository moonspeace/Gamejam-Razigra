#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LaserTraceActor.generated.h"

class UMaterialInstanceDynamic;
class UStaticMeshComponent;

/** Short-lived cosmetic cylinder aligned from the weapon bone to the camera trace impact. */
UCLASS(NotBlueprintable, Transient)
class GAMEJAMRAZIGRA_API ALaserTraceActor : public AActor
{
    GENERATED_BODY()

public:
    ALaserTraceActor();
    virtual void Tick(float DeltaSeconds) override;
    void InitializeLaser(const FVector& Start, const FVector& End);

private:
    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UStaticMeshComponent> LaserMesh;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> LaserMaterial;

    float Elapsed = 0.0f;
    float Lifetime = 0.12f;
};
