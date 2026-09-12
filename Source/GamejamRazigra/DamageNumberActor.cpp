#include "DamageNumberActor.h"

#include "Camera/PlayerCameraManager.h"
#include "Components/SceneComponent.h"
#include "Components/TextBlock.h"
#include "Components/WidgetComponent.h"
#include "Engine/Font.h"
#include "GameFramework/PlayerController.h"
#include "GlobalGameData.h"

ADamageNumberActor::ADamageNumberActor()
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = false;
    SetActorEnableCollision(false);

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);
    DamageWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("DamageWidget"));
    DamageWidget->SetupAttachment(SceneRoot);
    DamageWidget->SetWidgetSpace(EWidgetSpace::World);
    DamageWidget->SetDrawSize(FVector2D(256.0f, 96.0f));
    DamageWidget->SetDrawAtDesiredSize(false);
    DamageWidget->SetPivot(FVector2D(0.5f, 0.5f));
    DamageWidget->SetTwoSided(true);
    DamageWidget->SetWorldScale3D(FVector(0.3f));
    DamageWidget->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    DamageWidget->SetTranslucentSortPriority(1000);
}

void ADamageNumberActor::InitializeDamageNumber(float DamageAmount, APlayerController* LocalController)
{
    const UGlobalGameData* Data = UGlobalGameData::Get(this);
    ViewController = LocalController;
    BaseColor = Data->DamageNumberColor;
    Lifetime = FMath::Max(0.05f, Data->DamageNumberLifetime);
    RiseSpeed = Data->DamageNumberRiseSpeed;

    DamageText = NewObject<UTextBlock>(this, TEXT("FloatingDamageText"));
    DamageText->SetText(FText::AsNumber(FMath::RoundToInt(DamageAmount)));
    DamageText->SetJustification(ETextJustify::Center);
    DamageText->SetColorAndOpacity(FSlateColor(BaseColor));
    DamageText->SetShadowOffset(FVector2D(2.0f, 2.0f));
    DamageText->SetShadowColorAndOpacity(FLinearColor::Black);
    FSlateFontInfo FontInfo;
    FontInfo.Size = FMath::Max(24, FMath::RoundToInt(Data->DamageNumberWorldSize));
    if (UFont* Font = Data->DamageNumberFont.LoadSynchronous())
    {
        FontInfo.FontObject = Font;
    }
    DamageText->SetFont(FontInfo);
    DamageWidget->SetSlateWidget(DamageText->TakeWidget());
    FLinearColor GlowTint = BaseColor;
    GlowTint.R *= 6.0f;
    GlowTint.G *= 6.0f;
    GlowTint.B *= 6.0f;
    DamageWidget->SetTintColorAndOpacity(GlowTint);
    SetLifeSpan(Lifetime);
}

void ADamageNumberActor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    Elapsed += DeltaSeconds;
    AddActorWorldOffset(FVector::UpVector * RiseSpeed * DeltaSeconds);

    if (const APlayerController* Controller = ViewController.Get())
    {
        if (const APlayerCameraManager* Camera = Controller->PlayerCameraManager)
        {
            SetActorRotation((Camera->GetCameraLocation() - GetActorLocation()).Rotation());
        }
    }

    FLinearColor FadedColor = BaseColor;
    FadedColor.A *= 1.0f - FMath::Clamp(Elapsed / Lifetime, 0.0f, 1.0f);
    if (DamageText)
    {
        DamageText->SetColorAndOpacity(FSlateColor(FadedColor));
        FLinearColor GlowTint = FadedColor;
        GlowTint.R *= 6.0f;
        GlowTint.G *= 6.0f;
        GlowTint.B *= 6.0f;
        DamageWidget->SetTintColorAndOpacity(GlowTint);
    }
}
