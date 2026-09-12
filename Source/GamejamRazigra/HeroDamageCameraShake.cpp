#include "HeroDamageCameraShake.h"

#include "Shakes/PerlinNoiseCameraShakePattern.h"

UHeroDamageCameraShake::UHeroDamageCameraShake(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    UPerlinNoiseCameraShakePattern* Pattern = ObjectInitializer.CreateDefaultSubobject<UPerlinNoiseCameraShakePattern>(
        this, TEXT("DamageShakePattern"));
    SetRootShakePattern(Pattern);
    Pattern->Duration = 0.32f;
    Pattern->BlendInTime = 0.03f;
    Pattern->BlendOutTime = 0.14f;
    Pattern->LocationAmplitudeMultiplier = 2.5f;
    Pattern->LocationFrequencyMultiplier = 22.0f;
    Pattern->RotationAmplitudeMultiplier = 1.25f;
    Pattern->RotationFrequencyMultiplier = 18.0f;
    Pattern->Roll.Amplitude = 0.35f;
    Pattern->FOV.Amplitude = 0.25f;
}
