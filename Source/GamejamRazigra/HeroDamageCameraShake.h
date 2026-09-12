#pragma once

#include "Camera/CameraShakeBase.h"
#include "HeroDamageCameraShake.generated.h"

/** Small native impact shake used when no custom Blueprint shake is assigned. */
UCLASS(Blueprintable)
class GAMEJAMRAZIGRA_API UHeroDamageCameraShake : public UCameraShakeBase
{
    GENERATED_BODY()

public:
    UHeroDamageCameraShake(const FObjectInitializer& ObjectInitializer);
};
