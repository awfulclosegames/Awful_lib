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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spline Movement")
	float MoveBias = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spline Movement")
	float RotationBlendRate = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spline Movement")
	float MoveTensioning = -1.0f;

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
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spline Response")
	float MinMovementResponse = 0.1f;

	/// <summary>
	/// Movement response is how long (in seconds) it takes the character to start trying to follow new input. 
	/// By default it scales based on velocity so that it takes longer to respond when moving faster 
	/// </summary>
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spline Response")
	float MaxMovementResponse = 0.5f;

	/// <summary>
	/// A factor applied to new movements (either starting from a stop, or interrupting a current movement)
	/// </summary>
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spline Response")
	float LaunchForce = 1.5f;



	/// <summary>
	/// How much of a change of input is required to pick a new spline control point (dead zone). Range = 0..1
	/// </summary>
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spline Response")
	float ResponseTollerance = 0.01f;

	/// <summary>
	/// A weighting factor for how much urgency do we derive from deflection. Range = 0..1, higher number is more urgency for a given deflection
	/// </summary>
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spline Response")
	float DeflectionWeight = 0.75f;


	/// <summary>
	/// How urgent a change needs to be to interrupt the current movement spline segment rather than waiting till we reach the end of the current curve. Range = 0..1
	/// </summary>
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spline Response")
	float InterruptionUrgency = 0.75f;

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

	void UpdateSplinePoints(float DeltaT, const FVector& Input);
	void EvaluateNavigationSpline(float DeltaT);

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

	const float m_TimeUrgencyBlendFactor = 0.5f;

	bool bEnabledSplineUpdates = false;
#if !UE_BUILD_SHIPPING
	FVector m_DEBUG_PosAtStartOfUpdate;
	FVector m_DEBUG_ComputedVelocity;
	FVector m_DEBUG_ComputedAcceleration;
#endif
};
