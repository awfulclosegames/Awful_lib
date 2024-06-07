// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Spline/SDZ_KBSpline_DataTypes.h"


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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spline Movement")
	float ControlLookahead = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spline Movement")
	float MovementResponse = 0.5f;

	virtual void SetMovementMode(EMovementMode NewMovementMode, uint8 NewCustomMode = 0) override;

	virtual void PhysCustom(float deltaTime, int32 Iterations) override;

	virtual void BeginPlay() override;

	virtual void PhysWalking(float deltaTime, int32 Iterations) override;

	virtual void ControlledCharacterMove(const FVector& InputVector, float DeltaSeconds) override;

	void SetUseSpline(bool Value);
	bool GetUseSpline()const { return m_SplineWalk; }
private:

	void HandleHitSomethign(const FVector& adjustedVel, FVector& location, FHitResult& hit, float deltaTime);

	TObjectPtr<ASplineTestCharacter> m_Character;

	UPROPERTY()
	UKBSplineConfig* m_SplineConfig;
	FKBSplineState m_SplineState;

	FVector m_MoveTarget;

	float m_currentSplineTime = -1.0f;
	bool m_SplineWalk = false;
	bool m_ValidSpline = false;
	bool m_HalftiimeUpdateNeeded = true;

};
