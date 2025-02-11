// Copyright Strati D. Zerbinis 2025. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Spline/AC_KBSpline_DataTypes.h"


#include "AC_SplineMovementComponent.generated.h"


class ACharacter;



UCLASS()
class AWFUL_SPLINEMOVEMENT_API UAC_SplineMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()
public:
	UAC_SplineMovementComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spline Movement")
	float ControlLookahead = 1.0f;

	/// <summary>
	/// Movement response is how long (in seconds) it takes the character to start trying to follow new input. 
	/// By default it scales based on velocity so that it takes longer to respond when moving faster 
	/// </summary>
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spline Movement")
	float MinMovementResponse = 0.1f;

	/// <summary>
	/// Movement response is how long (in seconds) it takes the character to start trying to follow new input. 
	/// By default it scales based on velocity so that it takes longer to respond when moving faster 
	/// </summary>
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spline Movement")
	float MaxMovementResponse = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spline Movement")
	float MoveBias = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spline Movement")
	float RotationBlendRate = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spline Movement")
	float MoveTensioning = -1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spline Movement")
	bool bSplineWalk = false;


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


	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual FRotator ComputeOrientToMovementRotation(const FRotator& CurrentRotation, float DeltaTime, FRotator& DeltaRotation) const override;
	virtual void ApplyAccumulatedForces(float DeltaSeconds) override;

protected:
	// Adding a new point into the control point stream. virtual so derived classes can provide whatever point generating logic they like
	virtual FVector GenerateNewSplinePoint(float DeltaT, const FVector& Input);
	virtual float GetCurrentMovementReponseTime() const;

private:

	void UpdateSplinePoints(float DeltaT, const FVector& Input);
	void EvaluateNavigationSpline(float DeltaT);

	void MoveAlongRail(const FVector& MomentumDir, FVector& TargetOffset, float DeltaT);
	void ResetSplineState();

	void DebugDrawEvaluateForVelocity(float DeltaT);


	TObjectPtr<ACharacter> m_Character;

	UPROPERTY()
	UKBSplineConfig* m_SplineConfig;
	FKBSplineState m_SplineState;

	FVector m_CurrentMoveTarget;
	FVector m_SegmentChordDir;

	FRotator m_DesiredRotation;
	int m_LastValidSegment = 0;
	float m_CurrentSegLen = 1.0f;

	float m_Throttle = 0.0f;
	float m_LastRecordedSpeed = 0.0f;

	float m_CurrentResponseRate = 0.0f;

#if !UE_BUILD_SHIPPING
	FVector m_DEBUG_PosAtStartOfUpdate;
	FVector m_DEBUG_ComputedVelocity;
	FVector m_DEBUG_ComputedAcceleration;
#endif
};
