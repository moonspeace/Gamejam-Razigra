#include "DamageNumberActor.h"

#include "Camera/PlayerCameraManager.h"
#include "Components/TextRenderComponent.h"
#include "Engine/Font.h"
#include "GameFramework/PlayerController.h"
#include "GlobalGameData.h"

ADamageNumberActor::ADamageNumberActor()
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = false;
    SetActorEnableCollision(false);

    DamageText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("DamageText"));
    SetRootComponent(DamageText);
    DamageText->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
    DamageText->SetVerticalAlignment(EVerticalTextAligment::EVRTA_TextCenter);
    DamageText->SetCastShadow(false);
    DamageText->SetTranslucentSortPriority(100);
    DamageText->bAlwaysRenderAsText = true;
    DamageText->SetVisibility(true);
    DamageText->SetHiddenInGame(false);
}

void ADamageNumberActor::InitializeDamageNumber(float DamageAmount, APlayerController* LocalController)
{
    const UGlobalGameData* Data = UGlobalGameData::Get(this);
    ViewController = LocalController;
    BaseColor = Data->DamageNumberColor;
    Lifetime = FMath::Max(0.05f, Data->DamageNumberLifetime);
    RiseSpeed = Data->DamageNumberRiseSpeed;

    DamageText->SetText(FText::AsNumber(FMath::RoundToInt(DamageAmount)));
    DamageText->SetWorldSize(FMath::Max(44.0f, Data->DamageNumberWorldSize));
    DamageText->SetTextRenderColor(BaseColor.ToFColor(true));
    if (UFont* Font = Data->DamageNumberFont.LoadSynchronous())
    {
        DamageText->SetFont(Font);
    }
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
    DamageText->SetTextRenderColor(FadedColor.ToFColor(true));
}
