// Copyright Strati D. Zerbinis 2025. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Spline/AC_KBSpline_DataTypes.h"
#include "Containers/RingBuffer.h"


#include "AC_SplineMovementComponent.generated.h"


class ACharacter;



UCLASS()
class AWFUL_SPLINEMOVEMENT_API UAC_SplineMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()
public:
	UAC_SplineMovementComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Advandced Spline Movement")
	float ControlLookahead = 1.0f;

	/// <summary>
	/// How much do we anticipate curvature before a junction of segments, Range 1..-1
	/// 1 is perform all movement after the segment transitions (enter the junction aligned to the chord) 
	///         this ammounts to no aniticipation of the new movement direction
	/// 0 is share curvature evenly between segments. DEFAULT VALUE. 
	/// -1 is perform all curvature before the junction entry (so we enter the new segment aligned to it's chord
	///         this ammounts to total anticipation
	/// </summary>
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Advandced Spline Movement")
	float MoveBias = 0.0f;

	/// <summary>
	/// How quickly we align the rotation to the splines desired rotation
	/// </summary>
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Advandced Spline Movement")
	float RotationBlendRate = 0.25f;

	/// <summary>
	/// how tightly do we pull the spline in to the chord. Range is 1..-infinity
	/// 1 is fully pulled in to the chord, moving in a straight line
	/// 0 is natural curvature.
	/// -1 is expanded curvature, softer pull towards the spline. DEFAULT VALUE
	///     NOTE: you can make this value arbitrarily negative to get a wider travel of the spline
	///           but extremely negative values can produce odd and unwanted effects
	/// </summary>
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Advandced Spline Movement")
	float MoveTensioning = -1.0f;

	/// <summary>
	/// How much do we continue the input movement in the lookahead. Essentially how much do we assume the input would 
	/// continue to turn in the current direction in the future
	/// Range 0..1
	/// 0 is no extended curvature, assume we move in a striaght line towards the lookahead
	/// 1 is assume each step of the look ahead has as much offset from the previous as the new movement has from the previous frame
	/// </summary>
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Advandced Spline Movement")
	float InputCurveContinuationFactor = 0.7;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spline Movement")
	bool bSplineWalk = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spline Movement")
	bool bForcePlanerOnly = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spline Movement")
	bool bDisableWhenInAir = true;

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


	/// <summary>
	/// Movement response is how long (in seconds) it takes the character to start trying to follow new input. 
	/// By default it scales based on velocity so that it takes longer to respond when moving faster 
	/// </summary>
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spline Input")
	float MinMovementResponse = 0.1f;

	/// <summary>
	/// Movement response is how long (in seconds) it takes the character to start trying to follow new input. 
	/// By default it scales based on velocity so that it takes longer to respond when moving faster 
	/// </summary>
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spline Input")
	float MaxMovementResponse = 0.5f;

	/// <summary>
	/// A factor applied to new movements (either starting from a stop, or interrupting a current movement)
	/// </summary>
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spline Response")
	float LaunchForce = 1.5f;



	/// <summary>
	/// How much of a change of input is required to pick a new spline control point (dead zone). Range = 0..1
	/// </summary>
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spline Input")
	float ResponseTollerance = 0.01f;


	/// <summary>
	/// An arbitrary factor to apply to urgency for tuning purposes
	/// </summary>
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spline Input")
	float UrgencyFactor = 1.0f;

	/// <summary>
	/// How urgent a change needs to be to interrupt the current movement spline segment rather than waiting till we reach the end of the current curve. Range = 0..1
	/// </summary>
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spline Input")
	float InterruptionUrgency = 0.75f;

	///// <summary>
	///// How fast the point insertion point returns to the Max response rate in seconds to fully recover
	///// </summary>
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spline Input")
	//float RecoveryRate = 0.75f;

	/// <summary>
	/// How fast is urgency forgotten. Ranges from 0..1 zero being immediatly forget urgency, 1 being never forget urgency
	/// </summary>
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spline Input")
	float UrgencyStickiness = 0.5f;

	virtual void BeginPlay() override;

	virtual void ControlledCharacterMove(const FVector& InputVector, float DeltaSeconds) override;

	void SetUseSpline(bool Value);
	bool GetUseSpline()const { return bSplineWalk; }


	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual FRotator ComputeOrientToMovementRotation(const FRotator& CurrentRotation, float DeltaTime, FRotator& DeltaRotation) const override;
	virtual void ApplyAccumulatedForces(float DeltaSeconds) override;

protected:
	// Adding a new point into the control point stream. virtual so derived classes can provide whatever point generating logic they like
	virtual FVector GenerateNewSplinePoint(float DeltaT, float TargetTime, const FVector& Input);
	virtual float GetCurrentMovementReponseTime() const;

private:
	void HandleInterruption(FVector input, float DeltaSeconds);

	void UpdateSplinePoints(float DeltaT, const FVector& Input);
	void EvaluateNavigationSpline(float DeltaT);
	void StepSplineTarget(float DeltaT, const FVector& MomentumDir, float& outProjectedMomentum, FVector& outTarge, FVector& outOffset);


	void MoveAlongRail(const FVector& MomentumDir, FVector& TargetOffset, float DeltaT);
	void ResetSplineState(float DeltaSeconds = 0.0f);

	void DebugDrawEvaluateForVelocity(float DeltaSeconds);


	TObjectPtr<ACharacter> m_Character;

	UPROPERTY()
	TObjectPtr<UKBSplineConfig> m_SplineConfig;
	FKBSplineState m_SplineState;

	FVector m_CurrentMoveTarget;
	FVector m_SegmentChordDir;

	FRotator m_DesiredRotation;
	int m_LastValidSegment = 0;
	float m_CurrentSegLen = 1.0f;

	float m_Throttle = 0.0f;
	float m_LastRecordedSpeed = 0.0f;
	float m_UrgencyFactor = 0.0f;

	FVector m_CachedDeflection = FVector{ 0.0f };
	float m_TimeSinceLastDeflectionChange = 0.0f;
	float m_AccumulatedPressure = 0.0f;
	float m_AccumulatedNormalization = 0.0f;

	bool m_interrupted = false;
	const float m_InterruptionUrgencyReductionFactor = 0.5f;
	const float m_MaxDeltaVMultiplier = 1.5f; // 2.0 would be going from full speed one way to full speed 180 degrees. 75% of that is usually good

	bool bEnabledSplineUpdates = false;
#if !UE_BUILD_SHIPPING
	FVector m_DEBUG_PosAtStartOfUpdate;
	FVector m_DEBUG_ComputedVelocity;
	FVector m_DEBUG_ComputedAcceleration;
	int m_DEBUG_DrawnSegment = -1;
#endif
};
