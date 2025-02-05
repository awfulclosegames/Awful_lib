// Copyright Strati D. Zerbinis 2025. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Spline/AC_KBSpline_DataTypes.h"


#include "AC_SplineMovementComponent.generated.h"


class ASplineTestCharacter;



UCLASS()
class AWFUL_LIB_API UAC_SplineMovementComponent : public UCharacterMovementComponent
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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spline Movement")
	bool bSplineWalk = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General Movement")
	float Speed = 0.0f;

	/// <summary>
	/// How wide is the rail that the character is trying to stay on, or how precisely the character attempts to follow the spline
	/// </summary>
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spline Following")
	float RailWidth = 10.0f;

	/// <summary>
	/// Force Stay On Rail will ensure that the character does not step off of the spline. This may affect the speed that was set.
	/// This is more extreme than a Rail Width of 0, which can still step off the spline since it maintains chosen speed. 
	/// </summary>
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spline Following")
	bool bForceStayOnRail = false;



	virtual void BeginPlay() override;

	virtual void ControlledCharacterMove(const FVector& InputVector, float DeltaSeconds) override;
	
	void SetUseSpline(bool Value);
	bool GetUseSpline()const { return bSplineWalk; }

	void IncreaseResponse();
	void DecreaseResponse();

	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void CalcVelocity(float DeltaTime, float Friction, bool bFluid, float BrakingDeceleration) override;
	virtual FRotator ComputeOrientToMovementRotation(const FRotator& CurrentRotation, float DeltaTime, FRotator& DeltaRotation) const override;
	virtual void ApplyAccumulatedForces(float DeltaSeconds) override;

protected:

private:

	void UpdateSplinePoints(float DeltaT, const FVector& Input);

	void EvaluateNavigationSpline(float DeltaT);

	void MoveAlongRail(const FVector& MomentumDir, FVector& TargetOffset, float DeltaT);

	void DebugDrawEvaluateForVelocity(float DeltaT);

	void ResetSplineState();

	TObjectPtr<ASplineTestCharacter> m_Character;

	UPROPERTY()
	UKBSplineConfig* m_SplineConfig;
	FKBSplineState m_SplineState;

	FVector m_CurrentMoveTarget;
	FVector m_SegmentChordDir;

	int m_LastValidSegment = 0;
	float m_CurrentSegLen = 1.0f;

	float m_Throttle = 0.0f;

#if !UE_BUILD_SHIPPING
	FVector m_DEBUG_PosAtStartOfUpdate;
	FVector m_DEBUG_ComputedVelocity;
	FVector m_DEBUG_ComputedAcceleration;
#endif
};
