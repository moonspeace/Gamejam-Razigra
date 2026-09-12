#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "CreateRazigraDataCommandlet.generated.h"

/** One-shot editor commandlet used to create the required GlobalGameData asset. */
UCLASS()
class GAMEJAMRAZIGRA_API UCreateRazigraDataCommandlet : public UCommandlet
{
    GENERATED_BODY()

public:
    UCreateRazigraDataCommandlet();
    virtual int32 Main(const FString& Params) override;
};
