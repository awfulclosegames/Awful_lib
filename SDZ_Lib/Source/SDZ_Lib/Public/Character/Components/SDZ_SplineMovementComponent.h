// Copyright Strati D. Zerbinis 2025. All Rights Reserved.

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spline Movement")
	float MoveBias = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spline Movement")
	float MoveTensioning = -1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General Movement")
	float Speed = 0.0f;

	virtual void SetMovementMode(EMovementMode NewMovementMode, uint8 NewCustomMode = 0) override;

	virtual void PhysCustom(float deltaTime, int32 Iterations) override;

	virtual void BeginPlay() override;

	virtual void PerformMovement(float DeltaSeconds) override;
	virtual void PhysWalking(float deltaTime, int32 Iterations) override;

	virtual void ControlledCharacterMove(const FVector& InputVector, float DeltaSeconds) override;

	void SetUseSpline(bool Value);
	bool GetUseSpline()const { return m_SplineWalk; }

	void IncreaseResponse();
	void DecreaseResponse();

	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void CalcVelocity(float DeltaTime, float Friction, bool bFluid, float BrakingDeceleration) override;

protected:

private:

	void HandleHitSomethign(const FVector& adjustedVel, FVector& location, FHitResult& hit, float deltaTime);

	void UpdateSplinePoints(float DeltaT, const FVector& Input);

	//void EvaluateNavigationSpline(float DeltaT, FVector& outInput);
	void EvaluateNavigationSpline(float DeltaT);

	void ResetSplineState();

	TObjectPtr<ASplineTestCharacter> m_Character;

	UPROPERTY()
	UKBSplineConfig* m_SplineConfig;
	FKBSplineState m_SplineState;

	FVector m_NextPointTarget;

	FVector m_SegmentChordDir;

	int m_LastValidSegment = 0;

	float m_Throttle = 0.0f;
	float m_HalfRespRate = 0.0f;

	float m_CurrentSegLen = 1.0f;
	float m_SegmentVelHeur = 0.0f;

	//float m_currentSplineTime = -1.0f;
	bool m_SplineWalk = false;
	bool m_ValidSpline = false;

};
