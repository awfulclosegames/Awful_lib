// Copyright Strati D. Zerbinis 2025. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Character/Components/AC_SplineMovementComponent.h"
#include "Spline/AC_KBSpline_DataTypes.h"


#include "AC_TEST_SplineMovementComponent.generated.h"


class ACharacter;


UCLASS()
class AWFUL_LIB_API UAC_TEST_SplineMovementComponent : public UAC_SplineMovementComponent
{
	GENERATED_BODY()
public:
	UAC_TEST_SplineMovementComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	void IncreaseResponse();
	void DecreaseResponse();


protected:

private:
};
