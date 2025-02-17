// Copyright Strati D. Zerbinis 2025. All Rights Reserved.


#include "Character/Components/AC_SplineMovementComponent.h"
#include "MathUtil.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"
#include "VisualLogger/VisualLogger.h"
#include "DrawDebugHelpers.h"

#include "Spline/AC_KBSpline.h"


DEFINE_LOG_CATEGORY_STATIC(LogSplineMovement, Log, All);

static TAutoConsoleVariable<bool> CVarAC_SplineMoveDebug(TEXT("Awful.SplineMovement.Debug"), false, TEXT("Enable/Disable debug visualization for the Point of interest system"));


UAC_SplineMovementComponent::UAC_SplineMovementComponent(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void UAC_SplineMovementComponent::BeginPlay()
{
    Super::BeginPlay();
    m_Character = Cast<ACharacter>(GetOwner());
    ensure(IsValid(m_Character));

    m_CurrentResponseRate = MinMovementResponse;
    m_SplineConfig = UAC_KBSpline::CreateSplineConfig(m_Character->GetActorLocation() - (m_Character->GetActorForwardVector() * GetMaxSpeed() * m_CurrentResponseRate));

    ResetSplineState();
}

void UAC_SplineMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    m_CurrentResponseRate = GetCurrentMovementReponseTime();
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    m_LastRecordedSpeed = Velocity.Length();

#if !UE_BUILD_SHIPPING
    if (CVarAC_SplineMoveDebug.GetValueOnAnyThread())
    {
        DebugDrawEvaluateForVelocity(DeltaTime);
    }
#endif
}

void UAC_SplineMovementComponent::ControlledCharacterMove(const FVector& InputVector, float DeltaSeconds)
{
    FVector input = InputVector.GetClampedToMaxSize(1.0f);
    const float RequestedSpeedSquared = input.SizeSquared();
    m_TimeSinceLastDeflectionChange += DeltaSeconds;

    if (RequestedSpeedSquared > UE_KINDA_SMALL_NUMBER)
    {
        if (((input - m_CachedDeflection) * GetMaxSpeed()).SquaredLength() > FMath::Square(m_Tollerance))
        {
            m_CachedDeflection = input;
            // always assume we've at least taken one frame to respond

            m_TimeSinceLastDeflectionChange = DeltaSeconds;
            m_Throttle = m_CachedDeflection.Length() * GetMaxSpeed();
        }
        
        UpdateSplinePoints(DeltaSeconds, m_CachedDeflection);
        
#if !UE_BUILD_SHIPPING
        if (CVarAC_SplineMoveDebug.GetValueOnAnyThread())
        {
            if (bSplineWalk)
            {
                FVector RequestedTarget = input * GetMaxSpeed() * ControlLookahead;
                FVector MovementResponseTarget = input * GetMaxSpeed() * m_CurrentResponseRate;
                RequestedTarget.Z = 0.0f;
                MovementResponseTarget.Z = 0.0f;
                DrawDebugSphere(GetWorld(), m_Character->GetActorLocation() + RequestedTarget, 5.0f, 8, FColor::Black, false);
                DrawDebugSphere(GetWorld(), m_Character->GetActorLocation() + MovementResponseTarget, 5.0f, 8, FColor::Orange, false);

                UAC_KBSpline::DrawDebug(m_Character, m_SplineConfig, m_SplineState, FColor::Blue, 1.0f, 8.0f);
            }
        }
#endif
    }
    else
    {
        ResetSplineState();
    }
            m_TimeSinceLastDeflectionChange += DeltaSeconds;

    Super::ControlledCharacterMove(input, DeltaSeconds);
}

FVector UAC_SplineMovementComponent::GenerateNewSplinePoint(float DeltaT, const FVector& Input)
{
    FVector nextPointTarget = m_SplineConfig->ControlPoints.Last().Location;
    float urgencyAdjustment = m_TimeSinceLastDeflectionChange / m_CurrentResponseRate;

    nextPointTarget += (Input * GetMaxSpeed() * FMath::Clamp(m_TimeSinceLastDeflectionChange, MinMovementResponse, MaxMovementResponse));
    nextPointTarget.Z = m_Character->GetActorLocation().Z;
    return nextPointTarget;
}

float UAC_SplineMovementComponent::GetCurrentMovementReponseTime() const
{
    float effectiveMax = MaxMovementResponse - MinMovementResponse;
    float scale = Velocity.SquaredLength() / FMath::Square(GetMaxSpeed());
    return MinMovementResponse + scale * effectiveMax;
}

void UAC_SplineMovementComponent::UpdateSplinePoints(float DeltaT, const FVector& Input)
{
    m_SplineConfig->CommitPoint = 3;
    m_SplineConfig->ClearToCommitments();

    FVector nextPointTarget = GenerateNewSplinePoint(DeltaT, Input);
    // if we're within a rail width we aren't really needing to move, at least our move won't be reliable, since that's the margine of error
    if ((nextPointTarget - m_Character->GetActorLocation()).SquaredLength() > FMath::Square( RailWidth))
    {
        UAC_KBSpline::AddSplinePoint(m_SplineConfig, { nextPointTarget , MoveTensioning, MoveBias });
    }
}

// This method tries to follow the spline by sampling a point and moving towards it until it's too close, then sampling a new one by updating the 
//    chord time against the current character projected position or by adding a fixed 'quantum step'.
// an alternate and possibly better approach would by to use a critically damped spring to adjust the chord time update rate to self adjust in a way 
//    that converges on an ideal offset distance
void UAC_SplineMovementComponent::EvaluateNavigationSpline(float DeltaT)
{
    FVector momentumDir = Velocity.GetSafeNormal();
    float expectedTravel = m_Throttle * DeltaT;
    FVector targetOffset = m_CurrentMoveTarget - m_Character->GetActorLocation();
    float projectedMomentum = targetOffset.Dot(momentumDir);
    float chordNormalizedExpectedTravel = m_SegmentChordDir.Dot(momentumDir) * expectedTravel;

    // if we're too close, then try and update the point on the spline that we are following
    if (projectedMomentum < chordNormalizedExpectedTravel)
    {
        m_CurrentMoveTarget = m_Character->GetActorLocation();
        targetOffset = FVector::ZeroVector;

        // try and update the target point, potentially crossing up to 4 more segments
        for (int i = 0; i < 4; ++i)
        {
            if (m_SplineState.IsValidSegment())
            {
                // try and update the point within the segment
                float quantumUpdate = (DeltaT * 2.0f * m_LastRecordedSpeed) / m_CurrentSegLen;

                float candidateTime = m_SegmentChordDir.Dot((m_Character->GetActorLocation() - m_SplineState.WorkingSet[FKBSplineState::FromPoint].Location) + (momentumDir * expectedTravel)) / m_CurrentSegLen;
                m_SplineState.Time = quantumUpdate + FMath::Max(m_SplineState.Time, candidateTime);

                FVector candidateTarget = UAC_KBSpline::Sample(m_SplineState);
                FVector candidateOfset = candidateTarget - m_Character->GetActorLocation();
                
                projectedMomentum = candidateOfset.Dot(momentumDir);

                if (m_SplineState.Time <= 1.0f && projectedMomentum > chordNormalizedExpectedTravel)
                {
                    m_CurrentMoveTarget =  candidateTarget;
                    targetOffset = candidateOfset;
                    break;
                }
            }
            else
            {
                // try and get a new segment
                int proposedSegment = m_SplineConfig->GetNextCandidateSegment(m_SplineState.CurrentTraversalSegment);
  
                m_SplineState = UAC_KBSpline::PrepareForEvaluation(m_SplineConfig, proposedSegment);
                UAC_KBSpline::GetChord(m_SplineConfig, proposedSegment, m_SegmentChordDir);
                m_CurrentSegLen = m_SegmentChordDir.Length();
                if (m_CurrentSegLen > 0.0f)
                {
                    m_SegmentChordDir /= m_CurrentSegLen;
                }
                int toConsume = m_SplineState.IsValidSegment() ? m_LastValidSegment : proposedSegment;
                m_SplineConfig->ConsumeSegment(toConsume);
           }
        }
    }

    MoveAlongRail(momentumDir, targetOffset, DeltaT);

    if (m_SplineState.IsValidSegment() && DeltaT > 0.0f)
        m_LastValidSegment = m_SplineState.CurrentTraversalSegment;
}



void UAC_SplineMovementComponent::MoveAlongRail(const FVector& MomentumDir, FVector& TargetOffset, float DeltaT)
{
    // if we're not moving, don't bother
    if (TargetOffset.SquaredLength() > 0.0f)
    {
#if !UE_BUILD_SHIPPING
        m_DEBUG_PosAtStartOfUpdate = m_Character->GetActorLocation();
#endif
        // we're locking to the rail so clamp the movement 
        if (bForceStayOnRail)
        {
            if (DeltaT > 0.0f)
            {
                float offsetDist = TargetOffset.Length();
                float travelDist = FMath::Min(offsetDist / DeltaT, m_Throttle);

                Acceleration = ((TargetOffset / offsetDist) * travelDist) / DeltaT;
#if !UE_BUILD_SHIPPING
                m_DEBUG_ComputedVelocity = (TargetOffset / offsetDist) * travelDist;
                m_DEBUG_ComputedAcceleration = Acceleration;
#endif
            }
            return;
        }

        // Try and aim at the nearest point on the (potentially non-zero width) rail to try and travel to. 
        // Travel will be at the current set movement speed so the character may still overshoot or fall short of the rail
        FVector errorVec = (MomentumDir * MomentumDir.Dot(TargetOffset)) - TargetOffset;

        // if the error is more than the width of the rail, then aim for the near edge of the rail
        if (errorVec.SquaredLength() > FMath::Square(RailWidth * 0.5f))
        {
            // can do the unsafe normalize since we know it's length is non-zero from the conditional
            errorVec = errorVec.GetUnsafeNormal() * RailWidth * 0.5f;
        }

#if !UE_BUILD_SHIPPING
        if (CVarAC_SplineMoveDebug.GetValueOnAnyThread())
        {
            FVector debug_RailWidthVec = errorVec.GetSafeNormal() * RailWidth * 0.5f;
            UE_VLOG_SEGMENT_THICK(GetOwner(), LogSplineMovement, Verbose, m_DEBUG_PosAtStartOfUpdate + TargetOffset, m_DEBUG_PosAtStartOfUpdate + TargetOffset + debug_RailWidthVec, FColor::Black, 5.0f, TEXT("Rail"));           
            UE_VLOG_SEGMENT_THICK(GetOwner(), LogSplineMovement, Verbose, m_DEBUG_PosAtStartOfUpdate + TargetOffset, m_DEBUG_PosAtStartOfUpdate + errorVec, FColor::Cyan, 1.0f, TEXT("Intention"));
            UE_VLOG_SEGMENT_THICK(GetOwner(), LogSplineMovement, Verbose, m_DEBUG_PosAtStartOfUpdate + TargetOffset, m_DEBUG_PosAtStartOfUpdate, FColor::Orange, 4.0f, TEXT("Goal"));
            UE_VLOG_SEGMENT_THICK(GetOwner(), LogSplineMovement, Verbose, m_DEBUG_PosAtStartOfUpdate + TargetOffset, m_DEBUG_PosAtStartOfUpdate + TargetOffset + errorVec, FColor::Red, 2.0f, TEXT(""));
        }
#endif
        TargetOffset += errorVec;

        if (DeltaT > 0.0f)
        {
            const float MaxInputSpeed = FMath::Max(GetMaxSpeed() * AnalogInputModifier, GetMinAnalogSpeed());
            Velocity = ((TargetOffset.GetSafeNormal() * m_Throttle)).GetClampedToMaxSize(MaxInputSpeed);
        }

#if !UE_BUILD_SHIPPING
        m_DEBUG_ComputedVelocity = TargetOffset.GetSafeNormal() * m_Throttle;
        m_DEBUG_ComputedAcceleration = Acceleration;
#endif
    }
}


void UAC_SplineMovementComponent::ResetSplineState()
{
#if !UE_BUILD_SHIPPING
    UE_VLOG(GetOwner(), LogSplineMovement, Verbose, TEXT("   Resetting Spline!"));
#endif

    UAC_KBSpline::Reset(m_SplineConfig);

    // seed the empty spline with our current facing. 
    // NOTE:
    //  1) we assume the current position has a bias of 1 since we want all curvature for aligning to the next travel direction to occur after our current location
    //      since we're already here
    //  2) we could better approximate the correction to our new travel vector by taking our current velocity (if non-zero) and only usying facing if stationary
    UAC_KBSpline::AddSplinePoint(m_SplineConfig, { m_Character->GetActorLocation() - (m_Character->GetActorForwardVector() * GetMaxSpeed() * MaxMovementResponse) , MoveTensioning, MoveBias });
    UAC_KBSpline::AddSplinePoint(m_SplineConfig, { m_Character->GetActorLocation(), MoveTensioning, 1.0f });

    m_SplineState.CurrentTraversalSegment = 0;
    m_CurrentMoveTarget = m_Character->GetActorLocation();

    m_CurrentSegLen = 1.0f;
}



void UAC_SplineMovementComponent::DebugDrawEvaluateForVelocity(float DeltaT)
{
#if !UE_BUILD_SHIPPING

    UE_VLOG(GetOwner(), LogSplineMovement, Verbose, TEXT("*****************************\n        TICK\n*****************************\n        Segment: %i (isValid: %s)\n        Commit: %i\n        m_CurrentMoveTarget: %s\n        targetOffset: %s\n         Current Time: %f\n*****************************"),
        m_SplineState.CurrentTraversalSegment, (m_SplineConfig->IsValidSegment(m_SplineState.CurrentTraversalSegment) ? TEXT("True") : TEXT("False")), m_SplineConfig->CommitPoint,
        *m_CurrentMoveTarget.ToString(), *(m_CurrentMoveTarget - m_Character->GetActorLocation()).ToString(),
        m_SplineState.Time
    );


    UE_VLOG(GetOwner(), LogSplineMovement, Verbose, TEXT("        Velocity: %s\n        Acceleration: %s\n        DeltaT: %f\n        Computed Velocity: %s\n        Computed Acceleration: %s"),
        *Velocity.ToString(), *Acceleration.ToString(), DeltaT, *m_DEBUG_ComputedVelocity.ToString(), *m_DEBUG_ComputedAcceleration.ToString());


    UE_VLOG_LOCATION(GetOwner(), LogSplineMovement, Verbose, m_SplineState.WorkingSet[FKBSplineState::FromPoint].Location, 1.0f, FColor::Green, TEXT("From"));
    UE_VLOG_LOCATION(GetOwner(), LogSplineMovement, Verbose, m_SplineState.WorkingSet[FKBSplineState::ToPoint].Location, 1.0f, FColor::Red, TEXT("To"));

    UE_VLOG_LOCATION(GetOwner(), LogSplineMovement, Verbose, m_DEBUG_PosAtStartOfUpdate, 1.0f, FColor::Blue, TEXT("Loc"));
    UE_VLOG_SEGMENT_THICK(GetOwner(), LogSplineMovement, Verbose, m_DEBUG_PosAtStartOfUpdate, m_DEBUG_PosAtStartOfUpdate + (Velocity * DeltaT), FColor::Black, 4.0f, TEXT(""));



    UE_VLOG_SEGMENT_THICK(GetOwner(), LogSplineMovement, Verbose, m_DEBUG_PosAtStartOfUpdate, m_DEBUG_PosAtStartOfUpdate + (Velocity * DeltaT), FColor::Purple, 3.0f, TEXT("Vel Step"));
    UE_VLOG_LOCATION(GetOwner(), LogSplineMovement, Verbose, m_DEBUG_PosAtStartOfUpdate + (Velocity * DeltaT), 1.0f, FColor::Red, TEXT("Vel Step"));

#endif
}


void UAC_SplineMovementComponent::SetUseSpline(bool Value)
{
    bool prevValue = bSplineWalk;
    bSplineWalk = Value;
    if (bSplineWalk && !prevValue)
    {
        ResetSplineState();
    }
}


FRotator UAC_SplineMovementComponent::ComputeOrientToMovementRotation(const FRotator& CurrentRotation, float DeltaTime, FRotator& DeltaRotation) const
{
    if (!bSplineWalk)
    {
        return Super::ComputeOrientToMovementRotation(CurrentRotation, DeltaTime, DeltaRotation);
    }

    auto nextRotation = FMath::Lerp(CurrentRotation, m_DesiredRotation, RotationBlendRate);
    return nextRotation;
}

void UAC_SplineMovementComponent::ApplyAccumulatedForces(float DeltaSeconds)
{
    Super::ApplyAccumulatedForces(DeltaSeconds);

    if (bSplineWalk)
    {
        EvaluateNavigationSpline(DeltaSeconds);
        if (m_SplineState.IsValidSegment())
        {
            m_DesiredRotation = Velocity.GetSafeNormal().Rotation();
        }
    }
}

