// Copyright Strati D. Zerbinis 2025. All Rights Reserved.


#include "Character/Components/AC_TEST_SplineMovementComponent.h"
#include "MathUtil.h"
#include "GameFramework/Character.h"

#include "VisualLogger/VisualLogger.h"
#include "DrawDebugHelpers.h"


UAC_TEST_SplineMovementComponent::UAC_TEST_SplineMovementComponent(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void UAC_TEST_SplineMovementComponent::DecreaseResponse()
{
    MovementResponse *= 0.75f;

}


void UAC_TEST_SplineMovementComponent::IncreaseResponse()
{
    MovementResponse /= 0.75f;

}

