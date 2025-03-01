// Copyright Strati D. Zerbinis 2025. All Rights Reserved.


#include "Character/Components/AC_SplineMovementComponent.h"
#include "MathUtil.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"
#include "VisualLogger/VisualLogger.h"
#include "DrawDebugHelpers.h"

#include "Spline/AC_KBSpline.h"

//UE_DISABLE_OPTIMIZATION
DEFINE_LOG_CATEGORY_STATIC(LogSplineMovement, Log, All);

static TAutoConsoleVariable<bool> CVarAC_SplineMoveDebug(TEXT("Awful.SplineMovement.Debug"), false, TEXT("Enable/Disable debug visualization for the Spline movement component"));
static TAutoConsoleVariable<bool> CVarAC_SplineDetailedMoveDebug(TEXT("Awful.SplineDetailedMovement.Debug"), false, TEXT("Enable/Disable detailed visualization for the Spline movement component"));


UAC_SplineMovementComponent::UAC_SplineMovementComponent(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void UAC_SplineMovementComponent::BeginPlay()
{
    Super::BeginPlay();
    m_Character = Cast<ACharacter>(GetOwner());
    ensure(IsValid(m_Character));

    m_SplineConfig = UAC_KBSpline::CreateSplineConfig(m_Character->GetActorLocation() - (m_Character->GetActorForwardVector() * GetMaxSpeed() * MaxMovementResponse));

    ResetSplineState();
}

void UAC_SplineMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
#if !UE_BUILD_SHIPPING
    m_DEBUG_PosAtStartOfUpdate = m_Character->GetActorLocation();
#endif
    
    m_interrupted = false;
    bEnabledSplineUpdates = bSplineWalk;
    if (bDisableWhenInAir)
    {
        bEnabledSplineUpdates = bEnabledSplineUpdates && !(MovementMode == MOVE_Falling || MovementMode == MOVE_Flying);
    }

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
    m_Throttle = m_LastRecordedSpeed;

    m_TimeSinceLastDeflectionChange += DeltaSeconds;

    if (bEnabledSplineUpdates && (RequestedSpeedSquared > UE_KINDA_SMALL_NUMBER))
    {
        float deflectionDeltaV = (1.0f - m_SegmentChordDir.Dot(input)) * GetMaxSpeed();

        if (deflectionDeltaV > (ResponseTollerance * GetMaxSpeed()))
        {
            m_CachedDeflection = input;
            // pressure is based on kinetic energy which is 0.5f * mass * vel^2. Assume a constant mass and pressure is proportional to the square of velocity
            // since the delta is computed from velocities it is already a square velocity... though in inconvenient units since energy is usually computed in m/s not cm/s
            m_AccumulatedPressure += FMath::Square(deflectionDeltaV);
            // normalized against the maximum possible delta V (from max speed in one direction to max speed in the opposite direction) and compensated for the accumulation
            // We don't actually want to normalize against the full delta v range since the top end is rarely used and it throws off the urgency so we use 75% of it (the coverage)
            m_AccumulatedNormalization += FMath::Square(m_MaxDeltaVMultiplier * GetMaxSpeed());

            // urgency based on accumulated energy, so more changes per second of greater delta v mean more urgency.
            // and limited to 1..0
            m_UrgencyFactor = (m_AccumulatedPressure) / m_AccumulatedNormalization;
            m_UrgencyFactor = FMath::Clamp(m_UrgencyFactor * UrgencyFactor, 0.0f, 1.0f);

            UE_VLOG(GetOwner(), LogSplineMovement, Verbose, TEXT("                         m_TimeSinceLastDeflectionChange: %f\n                         m_AccumulatedPressure: %f\n                                                  m_AccumulatedNormalization: %f\n                                                  current energy delta: %f\n                         m_UrgencyFactor: %f\n                         Previous Throttle: %f"),
                m_TimeSinceLastDeflectionChange, 
                m_AccumulatedPressure, m_AccumulatedNormalization, deflectionDeltaV,
                m_UrgencyFactor, m_Throttle);

            m_TimeSinceLastDeflectionChange = 0.0f;
            m_Throttle = m_CachedDeflection.Length() * GetMaxSpeed();

            HandleInterruption(input, DeltaSeconds);
        }

        m_AccumulatedPressure *= UrgencyStickiness; // decay the pressure with time
        m_AccumulatedNormalization *= UrgencyStickiness;
        UpdateSplinePoints(DeltaSeconds, m_CachedDeflection);

#if !UE_BUILD_SHIPPING
        if (CVarAC_SplineMoveDebug.GetValueOnAnyThread())
        {
            if (bSplineWalk)
            {
                UE_VLOG_SEGMENT_THICK(GetOwner(), LogSplineMovement, Verbose, m_DEBUG_PosAtStartOfUpdate, m_DEBUG_PosAtStartOfUpdate + (input * 100), FColor::White, 8.0f, TEXT("Input (DeltaV: %f)"), deflectionDeltaV);
                UE_VLOG_SEGMENT_THICK(GetOwner(), LogSplineMovement, Verbose, m_DEBUG_PosAtStartOfUpdate, m_DEBUG_PosAtStartOfUpdate + (m_SegmentChordDir * 100), FColor::Yellow, 8.0f, TEXT("Current Travel Target"));
                
                FVector RequestedTarget = input * GetMaxSpeed() * ControlLookahead;
                FVector MovementResponseTarget = input * GetMaxSpeed() * (DeltaSeconds + GetCurrentMovementReponseTime());
                RequestedTarget.Z = 0.0f;
                MovementResponseTarget.Z = 0.0f;
                FColor urgencyColor = m_interrupted ? FColor::Red : FColor::Blue;
                DrawDebugSphere(GetWorld(), m_Character->GetActorLocation() + RequestedTarget, 5.0f, 8, FColor::Black, false);
                DrawDebugSphere(GetWorld(), m_Character->GetActorLocation() + MovementResponseTarget, 5.0f, 8, FColor::Orange, false);

                if (m_DEBUG_DrawnSegment != m_SplineState.CurrentTraversalSegment)
                {
                    m_DEBUG_DrawnSegment = m_SplineState.CurrentTraversalSegment;
                    UAC_KBSpline::DrawDebug(m_Character, m_SplineConfig, m_SplineState, urgencyColor, 1.0f, 8.0f);
                }
            }
        }
#endif
    }
    else
    {
        ResetSplineState(DeltaSeconds);
    }

    Super::ControlledCharacterMove(input, DeltaSeconds);
}

void UAC_SplineMovementComponent::HandleInterruption(FVector input, float DeltaSeconds)
{
    // if there's no urgency, than favour smoothness and stick to the standard response ranges
    if ((m_UrgencyFactor > InterruptionUrgency) && m_SplineState.IsValidSegment())
    {
        // we know there is some urgency, the next question: is the current segment too long?
        float timeToMoveTarget = (m_CurrentMoveTarget - m_Character->GetActorLocation()).Length() / m_LastRecordedSpeed;
        float targetResponseTime = FMath::Max(DeltaSeconds, MinMovementResponse) + ((MaxMovementResponse - MinMovementResponse) * (1.0f - m_UrgencyFactor));
        float junctionSplineTime = m_SplineState.Time + FMath::Max(0.0f, ((targetResponseTime - timeToMoveTarget) * m_LastRecordedSpeed) / (m_CurrentSegLen + UE_SMALL_NUMBER) );
        // estimate when we need to junction to the new movement request. Assume we can continue to respect the minimum movement response time

        if (junctionSplineTime < 1.0f)
        {
            FVector junctionPoint = UAC_KBSpline::SampleExplicit(m_SplineState, junctionSplineTime);

            m_interrupted = true;
            m_TimeSinceLastDeflectionChange = (LaunchForce * (m_LastRecordedSpeed / GetMaxSpeed()));

            // drop the urgency since we're immediatly switching tracks
            m_UrgencyFactor *= m_InterruptionUrgencyReductionFactor;
            m_AccumulatedPressure *= m_InterruptionUrgencyReductionFactor;
            m_AccumulatedNormalization *= m_InterruptionUrgencyReductionFactor;

            ResetSplineState(DeltaSeconds);

            float interruptionBias = FMath::Clamp((m_UrgencyFactor - InterruptionUrgency) / ((1.0f - InterruptionUrgency) + UE_SMALL_NUMBER), 0.0f, 1.0f);
            UAC_KBSpline::AddSplinePoint(m_SplineConfig, { junctionPoint , MoveTensioning, interruptionBias });

#if !UE_BUILD_SHIPPING
            if (CVarAC_SplineMoveDebug.GetValueOnAnyThread())
            {
                UE_VLOG(GetOwner(), LogSplineMovement, Verbose, TEXT(" Interrupting current spline segment!\n            moving off at junction point %s"), *junctionPoint.ToString());
                UE_VLOG(GetOwner(), LogSplineMovement, Verbose, TEXT("            targetResponseTime %f"), 
                    targetResponseTime);
                
                UE_VLOG_LOCATION(GetOwner(), LogSplineMovement, Verbose, m_Character->GetActorLocation() + (Velocity * targetResponseTime), 15.0f, FColor::White, TEXT("Interruption"));
                UE_VLOG_LOCATION(GetOwner(), LogSplineMovement, Verbose, junctionPoint, 15.0f, FColor::Purple, TEXT("Junction (%f)"), interruptionBias);
            }
#endif
        }
    }
}

FVector UAC_SplineMovementComponent::GenerateNewSplinePoint(float DeltaT, float TargetTime, const FVector& Input)
{
    FVector nextPointTarget = m_SplineConfig->ControlPoints.Last().Location;

    nextPointTarget += Input * GetMaxSpeed() * (DeltaT + TargetTime);
    nextPointTarget.Z = m_Character->GetActorLocation().Z;
    return nextPointTarget;
}

float UAC_SplineMovementComponent::GetCurrentMovementReponseTime() const
{
    return FMath::Clamp((MinMovementResponse + m_TimeSinceLastDeflectionChange) * (1.0f - m_UrgencyFactor), MinMovementResponse, MaxMovementResponse);
}

void UAC_SplineMovementComponent::UpdateSplinePoints(float DeltaT, const FVector& Input)
{
    m_SplineConfig->ClearToCommitments();

    float targetTime = GetCurrentMovementReponseTime();

    FVector nextPointTarget = GenerateNewSplinePoint(DeltaT, targetTime, Input);
    // if we're within a rail width we aren't really needing to move, at least our move won't be reliable, since that's the margine of error
    if ((nextPointTarget - m_Character->GetActorLocation()).SquaredLength() > FMath::Square(RailWidth))
    {
        UAC_KBSpline::AddSplinePoint(m_SplineConfig, { nextPointTarget , MoveTensioning, MoveBias });

        // can add additional look ahead points for managing things like Motion Matching here. Note still respect constraints
        float maxLookahead = ControlLookahead - targetTime;

        const FQuat baseRotation = Velocity.ToOrientationRotator().Quaternion();
        const FQuat inputRotation = Input.ToOrientationRotator().Quaternion();
        const FQuat deltaRotation = (inputRotation * baseRotation.Inverse()) * InputCurveContinuationFactor;
        FVector stepDir = Input;
        while (maxLookahead > 0.0f)
        {
            stepDir = deltaRotation.RotateVector(stepDir);
            float lookaheadStep = (targetTime + MaxMovementResponse) * 0.5f;
            lookaheadStep = FMath::Min(lookaheadStep, ControlLookahead);
            maxLookahead -= lookaheadStep;
            nextPointTarget = GenerateNewSplinePoint(DeltaT, lookaheadStep, stepDir);
            UAC_KBSpline::AddSplinePoint(m_SplineConfig, { nextPointTarget , MoveTensioning, MoveBias });
        }
    }
    m_SplineConfig->CommitPoint = 3;
}

// This method tries to follow the spline by sampling a point and moving towards it until it's too close, then sampling a new one by updating the 
//    chord time against the current character projected position or by adding a fixed 'quantum step'.
// an alternate and possibly better approach would by to use a critically damped spring to adjust the chord time update rate to self adjust in a way 
//    that converges on an ideal offset distance
void UAC_SplineMovementComponent::EvaluateNavigationSpline(float DeltaT)
{
    FVector momentumDir = Velocity.GetSafeNormal();
    FVector targetOffset = m_CurrentMoveTarget - m_Character->GetActorLocation();
    if (bForcePlanerOnly)
    {
        momentumDir.Z = 0.0f;
        targetOffset.Z = 0.0f;
    }

    float projectedMomentum = targetOffset.Dot(momentumDir);
    float chordNormalizedExpectedTravel = m_SegmentChordDir.Dot(momentumDir) * (m_Throttle * DeltaT);

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
                FVector candidateTarget;
                FVector candidateTangent;
                FVector candidateOfset;

                // may need to step multiple times through the spline to get a sample point relatively ahead of the character. cubic splines can 'lean' backwards 
                // if the chords meet at a highly acute angle
                StepSplineTarget(DeltaT, momentumDir, projectedMomentum, candidateTarget, candidateTangent, candidateOfset);
                if (m_SplineState.Time <= 1.0f && projectedMomentum > chordNormalizedExpectedTravel)
                {
                    m_CurrentMoveTarget = candidateTarget;
                    m_CurrentMoveTangent = candidateTangent;
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

void UAC_SplineMovementComponent::StepSplineTarget(float DeltaT, const FVector& MomentumDir, float& outProjectedMomentum, FVector& outTarge, FVector& outTangent, FVector& outOffset)
{
    const FVector& fromPoint = m_SplineState.WorkingSet[FKBSplineState::FromPoint].Location;
    FVector currentMoveTarget = m_CurrentMoveTarget;
    FVector tangent = outTangent;
    FVector currentOffset = outOffset;
    float currentProjectedMomentum = outProjectedMomentum;
    // try and update the point within the segment
    float quantumUpdate = (DeltaT * m_LastRecordedSpeed) / m_CurrentSegLen;
    bool stillTrying = true;
    while (stillTrying && m_SplineState.Time <= 1.0f)
    {
        // get the current position on the curve, get tangent here, and use that to estimate the next step along the curve. Then project that against 
        // the chord and normalize to get the new update time (make sure it doesn't go backwards too)
        tangent = UAC_KBSpline::ComputeTangent(m_SplineState);        
        FVector normalizedTargetPoint = currentMoveTarget + (tangent * DeltaT) - fromPoint;
        FVector normalizedExpectedStep = normalizedTargetPoint;
        float candidateTime = m_SegmentChordDir.Dot(normalizedExpectedStep) / m_CurrentSegLen;
        m_SplineState.Time = FMath::Max(m_SplineState.Time + quantumUpdate, candidateTime);

        currentMoveTarget = UAC_KBSpline::Sample(m_SplineState);
        currentOffset = currentMoveTarget - m_Character->GetActorLocation();

        // this part is hacky, should refactor this for a cleaner (and less branchy) flow for ignoring Z offsets
        if (bForcePlanerOnly)
        {
            currentOffset.Z = 0.0f;
        }
        currentProjectedMomentum = currentOffset.Dot(MomentumDir);

        float projectedPos = m_SegmentChordDir.Dot(m_Character->GetActorLocation() - fromPoint);
        float targetProjPos = m_SegmentChordDir.Dot(currentMoveTarget - fromPoint);

        // Either the new rabbit point should be ahead of us or farther along the spline. If neither of these is true step again
        stillTrying = (currentProjectedMomentum < 0.0f) && (projectedPos > targetProjPos);
    }
    outTarge = currentMoveTarget;
    outTangent = tangent;
    outOffset = currentOffset;
    outProjectedMomentum = currentProjectedMomentum;
}



void UAC_SplineMovementComponent::MoveAlongRail(const FVector& MomentumDir, FVector& TargetOffset, float DeltaSeconds)
{
    FVector targetMomentumDir = MomentumDir;
    // if we're not moving, don't bother
    if (TargetOffset.SquaredLength() > 0.0f)
    {
        // we're locking to the rail so clamp the movement 
        if (bForceStayOnRail)
        {
            if (DeltaSeconds > 0.0f)
            {
                float offsetDist = TargetOffset.Length();
                float travelDist = FMath::Min(offsetDist / DeltaSeconds, m_Throttle);

                Acceleration = ((TargetOffset / offsetDist) * travelDist) / DeltaSeconds;
#if !UE_BUILD_SHIPPING
                m_DEBUG_ComputedVelocity = (TargetOffset / offsetDist) * travelDist;
                m_DEBUG_ComputedAcceleration = Acceleration;
#endif
            }
            return;
        }

        FVector railDir = m_CurrentMoveTangent.Cross(m_Character->GetActorUpVector()).GetSafeNormal();

        // Try and aim at the nearest point on the (potentially non-zero width) rail to try and travel to. 
        // Travel will be at the current set movement speed so the character may still overshoot or fall short of the rail
        // 
        // compute the momentum rail crossing at the move target point do see if we're outside of tollerances
        FVector errorVec = railDir * ((MomentumDir * MomentumDir.Dot(TargetOffset)) - TargetOffset).Dot(railDir);
        bool headingCorrectionNeeded = errorVec.SquaredLength() > FMath::Square(RailWidth * 0.5f);
        // if the error is more than the width of the rail, then aim for the near edge of the rail
        if (headingCorrectionNeeded)
        {
            // can do the unsafe normalize since we know it's length is non-zero from the conditional
            errorVec = errorVec.GetUnsafeNormal() * RailWidth * 0.5f;
            targetMomentumDir = (TargetOffset + errorVec).GetSafeNormal();
        }

#if !UE_BUILD_SHIPPING
        if (CVarAC_SplineMoveDebug.GetValueOnAnyThread())
        {
         
            for (int wsPt = 0; wsPt < FKBSplineState::NumberOfPoints; ++wsPt)
            {
                UE_VLOG_LOCATION(GetOwner(), LogSplineMovement, Verbose, m_SplineState.WorkingSet[wsPt].Location, 6.0f, FColor::Orange, TEXT(""));
            }

            FVector debug_MomentumRailCrossing = railDir * ((MomentumDir * MomentumDir.Dot(TargetOffset)) - TargetOffset).Dot(railDir);
            FVector debug_RailWidthVec = errorVec.GetSafeNormal() * RailWidth * 0.5f;
            UE_VLOG_SEGMENT_THICK(GetOwner(), LogSplineMovement, Verbose, m_DEBUG_PosAtStartOfUpdate, m_DEBUG_PosAtStartOfUpdate + TargetOffset + debug_MomentumRailCrossing, FColor::Green, 5.0f, TEXT("Momentum Rail Crossing"));
            UE_VLOG_SEGMENT_THICK(GetOwner(), LogSplineMovement, Verbose, m_DEBUG_PosAtStartOfUpdate + TargetOffset, m_DEBUG_PosAtStartOfUpdate + TargetOffset + debug_RailWidthVec, FColor::Black, 10.0f, TEXT("Rail"));
            UE_VLOG_SEGMENT_THICK(GetOwner(), LogSplineMovement, Verbose, m_DEBUG_PosAtStartOfUpdate + TargetOffset, m_DEBUG_PosAtStartOfUpdate + errorVec, FColor::Cyan, 1.0f, TEXT("Intention"));
            UE_VLOG_SEGMENT_THICK(GetOwner(), LogSplineMovement, Verbose, m_DEBUG_PosAtStartOfUpdate + TargetOffset, m_DEBUG_PosAtStartOfUpdate, FColor::Orange, 4.0f, TEXT("Goal"));
            UE_VLOG_SEGMENT_THICK(GetOwner(), LogSplineMovement, Verbose, m_DEBUG_PosAtStartOfUpdate + TargetOffset, m_DEBUG_PosAtStartOfUpdate + TargetOffset + errorVec, FColor::Red, 2.0f, TEXT("%f"), errorVec.Length());
        }
#endif

        Velocity = targetMomentumDir * m_Throttle;
      
#if !UE_BUILD_SHIPPING
        m_DEBUG_ComputedVelocity = Velocity;
        m_DEBUG_ComputedAcceleration = Acceleration;
#endif
    }
}


void UAC_SplineMovementComponent::ResetSplineState(float DeltaSeconds)
{
#if !UE_BUILD_SHIPPING
    UE_VLOG(GetOwner(), LogSplineMovement, Verbose, TEXT("   Resetting Spline!"));
    m_DEBUG_DrawnSegment = -1;
#endif

    UAC_KBSpline::Reset(m_SplineConfig);
    m_SplineState.Reset();

    // seed the empty spline with our current facing. 
    // NOTE:
    //  1) we assume the current position has a bias of 1 since we want all curvature for aligning to the next travel direction to occur after our current location
    //      since we're already here
    //  2) we could better approximate the correction to our new travel vector by taking our current velocity (if non-zero) and only usying facing if stationary
    float launchScale = FMath::Max(GetMaxSpeed() * LaunchForce, m_LastRecordedSpeed * LaunchForce);

    UAC_KBSpline::AddSplinePoint(m_SplineConfig, { m_Character->GetActorLocation() - (m_Character->GetActorForwardVector() * launchScale) , MoveTensioning, MoveBias });
    UAC_KBSpline::AddSplinePoint(m_SplineConfig, { m_Character->GetActorLocation(), MoveTensioning, 1.0f });

    m_SplineState.CurrentTraversalSegment = 0;
    m_CurrentMoveTarget = m_Character->GetActorLocation() + (Velocity * DeltaSeconds);
    m_SegmentChordDir = m_Character->GetActorForwardVector();
    m_CurrentSegLen = m_LastRecordedSpeed * DeltaSeconds;
}



void UAC_SplineMovementComponent::DebugDrawEvaluateForVelocity(float DeltaT)
{
#if !UE_BUILD_SHIPPING

    int ctrlIdx = 0;
    for (auto& ctrlpt : m_SplineConfig->ControlPoints)
    {
        UE_VLOG_LOCATION(GetOwner(), LogSplineMovement, Verbose, ctrlpt.Location, 10.0f, FColor::Black, TEXT("[%i]"), ctrlIdx++);
    }

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

    if (CVarAC_SplineDetailedMoveDebug.GetValueOnAnyThread())
    {
        auto lookaheadState = m_SplineState;
        int lookaheadSegment = lookaheadState.CurrentTraversalSegment + 1;
        while (m_SplineConfig->IsValidSegment(lookaheadSegment))
        {
            lookaheadState = UAC_KBSpline::PrepareForEvaluation(m_SplineConfig, lookaheadSegment);
            UAC_KBSpline::DrawDebug(m_Character, m_SplineConfig, lookaheadState, FColor::Yellow, 5.0f, 0.0f);
            lookaheadSegment = lookaheadState.CurrentTraversalSegment + 1;
        }

        FColor urgencyColor = m_interrupted ? FColor::Red : FColor::Green;
        FVector urgencyBarLoc = m_Character->GetActorLocation() + m_Character->GetActorRightVector() * 30.0f;

        UE_VLOG_SEGMENT_THICK(GetOwner(), LogSplineMovement, Verbose, urgencyBarLoc, urgencyBarLoc + (FVector::UpVector * m_UrgencyFactor * 200.0f), urgencyColor, 5.0f, TEXT("Urgency: %f"), m_UrgencyFactor);
        //DrawDebugLine(m_Character->GetWorld(), urgencyBarLoc, urgencyBarLoc + (FVector::UpVector * m_UrgencyFactor * 200.0f), urgencyColor, false, 1, 0 ,3.0f);
    }

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
    if (bEnabledSplineUpdates)
    {
        auto nextRotation = FMath::Lerp(CurrentRotation, m_DesiredRotation, RotationBlendRate);
        return nextRotation;
    }

    return Super::ComputeOrientToMovementRotation(CurrentRotation, DeltaTime, DeltaRotation);

}

void UAC_SplineMovementComponent::ApplyAccumulatedForces(float DeltaSeconds)
{
    Super::ApplyAccumulatedForces(DeltaSeconds);

    if (bEnabledSplineUpdates)
    {
        EvaluateNavigationSpline(DeltaSeconds);
        if (m_SplineState.IsValidSegment())
        {
            m_DesiredRotation = Velocity.Rotation();
        }
    }
}

