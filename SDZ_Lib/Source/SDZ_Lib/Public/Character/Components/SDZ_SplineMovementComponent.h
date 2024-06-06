// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "SDZ_SplineMovementComponent.generated.h"

/**
 * 
 */

class ASplineTestCharacter;



UCLASS()
class SDZ_LIB_API USDZ_SplineMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()
public:

	
	virtual void SetMovementMode(EMovementMode NewMovementMode, uint8 NewCustomMode = 0) override;

	virtual void PhysCustom(float deltaTime, int32 Iterations) override;

	virtual void BeginPlay() override;


private:

	void HandleHitSomethign(const FVector& adjustedVel, FVector& location, FHitResult& hit, float deltaTime);

	TObjectPtr<ASplineTestCharacter> m_Character;


};
