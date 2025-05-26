// Copyright Strati D. Zerbinis 2025. All Rights Reserved.


#include "Character/Components/AC_SplineMovementComponent.h"
#include "MathUtil.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"
#include "VisualLogger/VisualLogger.h"
#include "DrawDebugHelpers.h"

#include "Spline/AC_KBSpline.h"

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
    DesiredBreakingForce = BrakingFriction;
    ResetSplineState();
}

void UAC_SplineMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
#if !UE_BUILD_SHIPPING
    m_DEBUG_PosAtStartOfUpdate = m_Character->GetActorLocation();
#endif
    m_SplineFollowingAcceleration = FVector::ZeroVector;

    m_Interrupted = false;
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
    // the m_ThrottleNomralization has the side effect of increasing (or decreasing) the deflection delta and the accumulated pressure 
    // based on normalization. We may want to only apply this against the throttle computation (currentThrottleValue)
    FVector input = (InputVector * m_ThrottleNomralization).GetClampedToMaxSize(1.0f);
    const float RequestedSpeedSquared = input.SizeSquared();

    m_TimeSinceLastDeflectionChange += DeltaSeconds;
    float currentThrottleAccumulationDecay = 1.0f - ThrottleAccumulationDecay;

    if (bEnabledSplineUpdates && (RequestedSpeedSquared > UE_KINDA_SMALL_NUMBER))
    {
        float deflectionDeltaVSqr = ((input * GetMaxSpeed()) - Velocity).SquaredLength();

        if (deflectionDeltaVSqr > FMath::Square(ResponseTollerance * GetMaxSpeed()))
        {
            m_CachedDeflection = input.GetUnsafeNormal() * RequestedSpeedSquared;
            // pressure is based on kinetic energy which is 0.5f * mass * vel^2. Assume a constant mass and pressure is proportional to the square of velocity
            // since the delta is computed from velocities it is already a square velocity... though in inconvenient units since energy is usually computed in m/s not cm/s
            m_AccumulatedPressure += deflectionDeltaVSqr;
            // normalized against the maximum possible delta V (from max speed in one direction to max speed in the opposite direction) and compensated for the accumulation
            // We don't actually want to normalize against the full delta v range since the top end is rarely used and it throws off the urgency so we use 75% of it (the coverage)
            m_AccumulatedNormalization += FMath::Square(m_MaxDeltaVMultiplier * GetMaxSpeed());

            // urgency based on accumulated energy, so more changes per second of greater delta v mean more urgency.
            // and limited to 1..0
            m_UrgencyFactor = (m_AccumulatedPressure) / m_AccumulatedNormalization;
            m_UrgencyFactor = FMath::Clamp(m_UrgencyFactor * UrgencyFactor, 0.0f, 1.0f);

            constexpr float MinimumSpeedForLaunch = 30.0f; // arbitrary value less than (in cm/s) counts as starting from standing 
            m_Launching = m_Launching || ((m_LastRecordedSpeed <= MinimumSpeedForLaunch) && (m_TimeSinceLastDeflectionChange > (2.0f * MinMovementResponse)));

            m_TimeSinceLastDeflectionChange = 0.0f;
            HandleInterruption(m_CachedDeflection, DeltaSeconds);
        }

        // Update the throttle as long as there seems to be active input
        float currentThrottleValue = RequestedSpeedSquared * GetMaxSpeed();
        m_Throttle = FMath::Lerp(currentThrottleValue, m_AccumulatedThrottle, ThrottleEnertia);
        m_AccumulatedThrottle = m_Throttle;
        currentThrottleAccumulationDecay = 1.0f; // don't decay on an update where we set a throttle value

        m_AccumulatedPressure *= UrgencyStickiness; // decay the pressure with time
        m_AccumulatedNormalization *= UrgencyStickiness;
        UpdateSplinePoints(DeltaSeconds, m_CachedDeflection);

#if !UE_BUILD_SHIPPING
        if (CVarAC_SplineMoveDebug.GetValueOnAnyThread())
        {
            if (bSplineWalk)
            {
                UE_VLOG_SEGMENT_THICK(GetOwner(), LogSplineMovement, Verbose, m_DEBUG_PosAtStartOfUpdate, m_DEBUG_PosAtStartOfUpdate + (input * 100), FColor::White, 8.0f, TEXT("Input (DeltaV: %f)"), FMath::Square(deflectionDeltaVSqr));
                UE_VLOG_SEGMENT_THICK(GetOwner(), LogSplineMovement, Verbose, m_DEBUG_PosAtStartOfUpdate, m_DEBUG_PosAtStartOfUpdate + (m_SegmentChordDir * 100), FColor::Yellow, 8.0f, TEXT("Current Travel Target"));
                
                FVector RequestedTarget = input * GetMaxSpeed() * ControlLookahead;
                FVector MovementResponseTarget = input * GetMaxSpeed() * (DeltaSeconds + GetCurrentMovementReponseTime(MinMovementResponse, MaxMovementResponse));
                RequestedTarget.Z = 0.0f;
                MovementResponseTarget.Z = 0.0f;
                FColor urgencyColor = m_Interrupted ? FColor::Red : FColor::Blue;
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

    // decay the accumulation over time regardless of input 
    m_AccumulatedThrottle *= currentThrottleAccumulationDecay;

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

            m_Interrupted = true;
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
                
                UE_VLOG_LOCATION(GetOwner(), LogSplineMovement, Verbose, m_Character->GetActorLocation() + (Velocity * targetResponseTime), 15, FColor::White, TEXT("Interruption"));
                UE_VLOG_LOCATION(GetOwner(), LogSplineMovement, Verbose, junctionPoint, 15, FColor::Purple, TEXT("Junction (%f)"), interruptionBias);
            }
#endif
        }
    }
}

void UAC_SplineMovementComponent::SetMovementMode(EMovementMode NewMovementMode, uint8 NewCustomMode)
{
    Super::SetMovementMode(NewMovementMode, NewCustomMode);
    if (bDisableWhenInAir)
    {
        bEnabledSplineUpdates = bEnabledSplineUpdates && !(MovementMode == MOVE_Falling || MovementMode == MOVE_Flying);
    }
}

FVector UAC_SplineMovementComponent::GenerateNewSplinePoint(float DeltaT, float TargetTime, const FVector& Input)
{
    FVector nextPointTarget = m_SplineConfig->ControlPoints.Last().Location;
    float magnatude = GetMaxSpeed() * (DeltaT + TargetTime);
    nextPointTarget += Input * magnatude;
    nextPointTarget.Z = m_Character->GetActorLocation().Z;
    return nextPointTarget;
}

float UAC_SplineMovementComponent::GetCurrentMovementReponseTime(float Min, float Max) const
{
    return FMath::Clamp((Min + m_TimeSinceLastDeflectionChange) * (1.0f - m_UrgencyFactor), Min, Max);
}



void UAC_SplineMovementComponent::UpdateSplinePoints(float DeltaT, const FVector& Input)
{
    m_SplineConfig->ClearToCommitments();

    float targetTime = GetCurrentMovementReponseTime(MinMovementResponse, MaxMovementResponse);

    FVector nextPointTarget = GenerateNewSplinePoint(DeltaT, targetTime, Input);
    // enforce minimum spline point spacing
    if ((nextPointTarget - m_Character->GetActorLocation()).SquaredLength() > FMath::Square(MinimumSplinePointSpacing))
    {
        UAC_KBSpline::AddSplinePoint(m_SplineConfig, { nextPointTarget , MoveTensioning, MoveBias });

        FilloutLookahead(Input, targetTime, DeltaT);
    }
    m_SplineConfig->CommitPoint = 3;
}


void UAC_SplineMovementComponent::FilloutLookahead(const FVector& Input, float TargetTime, float DeltaT)
{
    // can add additional look ahead points for managing things like Motion Matching here. Note still respect constraints
    float maxLookahead = ControlLookahead - TargetTime;

    const FQuat inputRotation = Input.ToOrientationRotator().Quaternion();
    const FQuat prevMotionRotation = m_SegmentChordDir.ToOrientationRotator().Quaternion(); // seg chord hasn't been updated yet
    const float curveContinuation = m_Launching ? 0.0f : InputCurveContinuationFactor; // set delta rotation to identiy if we're launching
    FQuat rawDeltaRotation = inputRotation * prevMotionRotation.Inverse();
    // combine with the previous update's delta to bias consistency
    FQuat scaledDeltaRotation = FMath::Lerp(FQuat::Identity, (rawDeltaRotation + m_PrevDelta) * 0.5f, curveContinuation);
    m_PrevDelta = rawDeltaRotation;

    FVector stepDir = Input;
    float stepTime = FMath::Max(TargetTime, DeltaT);
    float stepMaxTime = FMath::Max(MaxMovementResponse, DeltaT);

    while (maxLookahead > 0.0f)
    {
        // A consideration here is to move the step time interpolation to the end of the loop, if we want to ensure that the 
        // first lookahead step is no longer than the first (can improve response in the zero minimum response time case)
        stepTime = FMath::Lerp(stepTime, stepMaxTime, InputLookaheadBlendout);
        stepDir = scaledDeltaRotation.RotateVector(stepDir);
        stepTime = FMath::Min(stepTime, maxLookahead);
        maxLookahead -= stepTime;
        m_CurrentLookaheadPoint = GenerateNewSplinePoint(DeltaT, stepTime, stepDir);
        UAC_KBSpline::AddSplinePoint(m_SplineConfig, { m_CurrentLookaheadPoint , MoveTensioning, MoveBias });
        scaledDeltaRotation = FMath::Lerp(scaledDeltaRotation, FQuat::Identity, InputCurveContinuationDecay); // no SLERP?!
    }
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
            // Current segment isn't good, try and find a good one
            if (!m_SplineState.IsValidSegment())
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
            // TEST AGAIN, We may have changed it above. This logic can be cleaned up
            // Current segment is ok, try and find a good rabbit position
            if (m_SplineState.IsValidSegment())
            {
                FVector candidateTarget;
                FVector candidateTangent;
                FVector candidateOfset;

                // may need to step multiple times through the spline to get a sample point relatively ahead of the character. cubic splines can 'lean' backwards 
                // if the chords meet at a highly acute angle
                bool goodRabbit = StepSplineTarget(DeltaT, momentumDir, projectedMomentum, candidateTarget, candidateTangent, candidateOfset);

                if (m_SplineState.Time <= 1.0f && goodRabbit)
                {
                    m_CurrentMoveTarget = candidateTarget;
                    m_CurrentMoveTangent = candidateTangent;
                    targetOffset = candidateOfset;
                    break;
                }
            }
        }

        if (m_SplineState.IsValidSegment() && DeltaT > 0.0f)
        {
            MoveAlongRail(momentumDir, targetOffset, DeltaT);
            m_LastValidSegment = m_SplineState.CurrentTraversalSegment;
        }
    }
}

// Run rabbit, run!
bool UAC_SplineMovementComponent::StepSplineTarget(float DeltaT, const FVector& MomentumDir, float& outProjectedMomentum, FVector& outTarge, FVector& outTangent, FVector& outOffset)
{
    const FVector& fromPoint = m_SplineState.WorkingSet[FKBSplineState::FromPoint].Location;
    FVector currentMoveTarget = m_CurrentMoveTarget;
    FVector tangent = outTangent;
    FVector currentOffset = outOffset;
    float currentProjectedMomentum = outProjectedMomentum;
    // try and update the point within the segment. The quantum update must be non-zero. Arbitrary min speed of 10 cm/s
    // To be fully proprer this should probably be min of last speed and throttle, then early out the loop if it's zero
    float quantumUpdate = (DeltaT * FMath::Max(m_LastRecordedSpeed, 10.0f)) / m_CurrentSegLen;
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

        // Either the new rabbit point should be ahead of us or farther along the spline. If neither of these is true step again (test for the opposite conditions)
        stillTrying = (currentProjectedMomentum < 0.0f) && (projectedPos > targetProjPos);

#if !UE_BUILD_SHIPPING
        if (CVarAC_SplineDetailedMoveDebug.GetValueOnAnyThread())
        {
            UE_VLOG_LOCATION(GetOwner(), LogSplineMovement, Verbose, currentMoveTarget, 3, stillTrying ? FColor::Black : FColor::Magenta, TEXT("Potential Rabbit"));
        }
#endif
    }
    outTarge = currentMoveTarget;
    outTangent = tangent;
    outOffset = currentOffset;
    outProjectedMomentum = currentProjectedMomentum;
    return !stillTrying;
}



void UAC_SplineMovementComponent::MoveAlongRail(const FVector& MomentumDir, FVector& TargetOffset, float DeltaSeconds)
{
    FVector targetMomentumDir = MomentumDir;
    // if we're not moving, don't bother
    if (TargetOffset.SquaredLength() > 0.0f)
    {
        m_Launching = false; // if this was a start then we've started!
        // we're locking to the rail so clamp the movement 
        if (bForceStayOnRail)
        {
            if (DeltaSeconds > 0.0f)
            {
                float offsetDist = TargetOffset.Length();
                float travelDist = FMath::Min(offsetDist / DeltaSeconds, m_Throttle);

                m_SplineFollowingAcceleration = ((TargetOffset / offsetDist) * travelDist) / DeltaSeconds;
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

        const float misalignmentFactor = targetMomentumDir.Dot(TargetOffset) < 0.0f * 0.65f; // add an error factor for going the wrong dirrection
        // 10% fudge factor. Corresponds to the other 10% to effectively make the rail walls a bit thicker
        const float elasticError = FMath::Max(misalignmentFactor, errorVec.SquaredLength() / FMath::Square(RailWidth * 0.5f * 0.9f));

        // if the error is more than the width of the rail, then we need a major headding correction. Aim for the near edge of the rail
        if (elasticError > 1.0f)
        {
            // can do the unsafe normalize since we know it's length is non-zero from the conditional
            errorVec = errorVec.GetUnsafeNormal() * RailWidth * 0.5f;
            targetMomentumDir = (TargetOffset + errorVec).GetSafeNormal();
        }
        else
        {
            // Minor headding corrections, elstic control. Use the weighted error to blend in the tangent
            // 10% fudge the other way
            FVector tangentDir = UAC_KBSpline::ComputeTangent(m_SplineState).GetSafeNormal(); // this is cachable!
            targetMomentumDir = FMath::Lerp(targetMomentumDir, tangentDir, elasticError * 1.1f);
        }

#if !UE_BUILD_SHIPPING
        if (CVarAC_SplineMoveDebug.GetValueOnAnyThread())
        {      
            for (int wsPt = 0; wsPt < FKBSplineState::NumberOfPoints; ++wsPt)
            {
                UE_VLOG_LOCATION(GetOwner(), LogSplineMovement, Verbose, m_SplineState.WorkingSet[wsPt].Location, 6, FColor::Orange, TEXT(""));
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
        // we work in accelerations since we don't want to completely stomp all other mvoement controls. Though this does leave opportunities
        // for drift or to miss targets depending on input acceleration and framerate. Should rework velocity accumulation or acceleration management
        // right now the acceleration is heavily biased to stick input, which can be a problem

        // "Estimate" what the Unreal character move component is going to do for applying accelerations with ground force
        // so we can generate an acceleration that should get us at least close to our desired velocity
        const FVector AccelDir = Acceleration.GetSafeNormal();
        // NOTE: This formula is cut and pasted from the Unreal movement compopnent (?!)
        // remove groundfriction along the acceleration vector then re-add the accleration in to the velocity estimate
        FVector estimatedCurrentVel = Velocity - ((Velocity - AccelDir * m_LastRecordedSpeed) * FMath::Min(DeltaSeconds * GroundFriction, 1.f));
        // now compute a move that assumes we also want to compensate for the ground friction drag

        m_SplineFollowingAcceleration = ((targetMomentumDir * (m_Throttle)) - estimatedCurrentVel) / DeltaSeconds;
        float maxAccel = GetMaxAcceleration();
        m_SplineFollowingAcceleration = m_SplineFollowingAcceleration.GetClampedToSize(-maxAccel, maxAccel);

#if !UE_BUILD_SHIPPING
        m_DEBUG_ComputedVelocity = targetMomentumDir * m_Throttle;
        m_DEBUG_ComputedAcceleration = Acceleration;
#endif
    }
}


void UAC_SplineMovementComponent::ResetSplineState(float DeltaSeconds)
{
#if !UE_BUILD_SHIPPING
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

    m_PrevDelta = FQuat::Identity;
    m_Launching = false;

    m_CurrentLookaheadPoint = m_CurrentMoveTarget;
}



void UAC_SplineMovementComponent::DebugDrawEvaluateForVelocity(float DeltaT)
{
#if !UE_BUILD_SHIPPING
    if (!bSplineWalk)
        return;

    int ctrlIdx = 0;
    for (auto& ctrlpt : m_SplineConfig->ControlPoints)
    {
        UE_VLOG_LOCATION(GetOwner(), LogSplineMovement, Verbose, ctrlpt.Location, 10, FColor::Black, TEXT("[%i]"), ctrlIdx++);
    }

    UE_VLOG(GetOwner(), LogSplineMovement, Verbose, TEXT("*****************************\n        TICK\n*****************************\n        Segment: %i (isValid: %s)\n        Commit: %i\n        m_CurrentMoveTarget: %s\n        targetOffset: %s\n         Current Time: %f\n*****************************"),
        m_SplineState.CurrentTraversalSegment, (m_SplineConfig->IsValidSegment(m_SplineState.CurrentTraversalSegment) ? TEXT("True") : TEXT("False")), m_SplineConfig->CommitPoint,
        *m_CurrentMoveTarget.ToString(), *(m_CurrentMoveTarget - m_Character->GetActorLocation()).ToString(),
        m_SplineState.Time
    );


    UE_VLOG(GetOwner(), LogSplineMovement, Verbose, TEXT("        Velocity: %s\n        Acceleration: %s\n        DeltaT: %f\n        Computed Velocity: %s\n        Computed Acceleration: %s"),
        *Velocity.ToString(), *Acceleration.ToString(), DeltaT, *m_DEBUG_ComputedVelocity.ToString(), *m_DEBUG_ComputedAcceleration.ToString());
    
    UE_VLOG(GetOwner(), LogSplineMovement, Verbose, TEXT("Character location at the start of tick:  %s\n                             Currently :  %s"), *m_DEBUG_PosAtStartOfUpdate.ToString(), *m_Character->GetActorLocation().ToString());

    UE_VLOG_LOCATION(GetOwner(), LogSplineMovement, Verbose, m_SplineState.WorkingSet[FKBSplineState::FromPoint].Location, 1, FColor::Green, TEXT("From"));
    UE_VLOG_LOCATION(GetOwner(), LogSplineMovement, Verbose, m_SplineState.WorkingSet[FKBSplineState::ToPoint].Location, 1, FColor::Red, TEXT("To"));

    UE_VLOG_LOCATION(GetOwner(), LogSplineMovement, Verbose, m_DEBUG_PosAtStartOfUpdate, 2, FColor::Blue, TEXT("Loc"));
    UE_VLOG_SEGMENT_THICK(GetOwner(), LogSplineMovement, Verbose, m_DEBUG_PosAtStartOfUpdate, m_DEBUG_PosAtStartOfUpdate + (Velocity * DeltaT), FColor::Black, 4, TEXT(""));



    UE_VLOG_SEGMENT_THICK(GetOwner(), LogSplineMovement, Verbose, m_DEBUG_PosAtStartOfUpdate, m_DEBUG_PosAtStartOfUpdate + (Velocity * DeltaT), FColor::Purple, 3, TEXT("Vel Step"));
    UE_VLOG_LOCATION(GetOwner(), LogSplineMovement, Verbose, m_DEBUG_PosAtStartOfUpdate + (Velocity * DeltaT), 1, FColor::Red, TEXT("Vel Step"));
    UE_VLOG_LOCATION(GetOwner(), LogSplineMovement, Verbose, m_DEBUG_PosAtStartOfUpdate + (m_DEBUG_ComputedVelocity * DeltaT), 1, FColor::Cyan, TEXT("Computed Vel Step"));
    UE_VLOG_LOCATION(GetOwner(), LogSplineMovement, Verbose, m_CurrentMoveTarget, 1, FColor::Purple, TEXT("Rabbit"));

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

        FColor urgencyColor = m_Interrupted ? FColor::Red : FColor::Green;
        FVector urgencyBarLoc = m_Character->GetActorLocation() + m_Character->GetActorRightVector() * 30.0f;

        UE_VLOG_SEGMENT_THICK(GetOwner(), LogSplineMovement, Verbose, urgencyBarLoc, urgencyBarLoc + (FVector::UpVector * m_UrgencyFactor * 200.0f), urgencyColor, 5.0f, TEXT("Urgency: %f"), m_UrgencyFactor);
        //DrawDebugLine(m_Character->GetWorld(), urgencyBarLoc, urgencyBarLoc + (FVector::UpVector * m_UrgencyFactor * 200.0f), urgencyColor, false, 1, 0 ,3.0f);
    }
    m_DEBUG_ComputedVelocity = FVector::ZeroVector;
    m_DEBUG_ComputedAcceleration = FVector::ZeroVector;
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
    if (bEnabledSplineUpdates && m_SplineState.IsValidSegment())
    {
        FRotator DeltaR = Velocity.Rotation() - CurrentRotation;
        FRotator ClampedDeltaR(
            FMath::Min(DeltaR.Pitch, RotationRate.Pitch),
            FMath::Min(DeltaR.Yaw, RotationRate.Yaw),
            FMath::Min(DeltaR.Roll, RotationRate.Roll)
        );
        return CurrentRotation + ClampedDeltaR;
    }

    return Super::ComputeOrientToMovementRotation(CurrentRotation, DeltaTime, DeltaRotation);
}

void UAC_SplineMovementComponent::PerformMovement(float DeltaTime)
{
    if (bEnabledSplineUpdates)
    {
        EvaluateNavigationSpline(DeltaTime);
    }
    Super::PerformMovement(DeltaTime);
}

void UAC_SplineMovementComponent::CalcVelocity(float DeltaTime, float Friction, bool bFluid, float BrakingDeceleration)
{
    // we want deceleration to break in 0.1s or the passed in deceleration, whichever is more
    float TimeToStop = GetCurrentMovementReponseTime(MinTimeToStop, MaxTimeToStop);
    DesiredBreakingForce = FMath::Max(m_LastRecordedSpeed / TimeToStop, BrakingDeceleration);
    Super::CalcVelocity(DeltaTime, Friction, bFluid, DesiredBreakingForce);

    Velocity += m_SplineFollowingAcceleration * DeltaTime;
}

void UAC_SplineMovementComponent::HandleImpact(const FHitResult& Hit, float TimeSlice, const FVector& MoveDelta)
{
    // If we're blocked, then drop the spline and just slide until we're unobstructed again
    if (Hit.IsValidBlockingHit())
    {
        ResetSplineState(TimeSlice);
    }
    Super::HandleImpact(Hit, TimeSlice, MoveDelta);
}

FVector UAC_SplineMovementComponent::GetLookaheadPoint() const
{
    return m_CurrentLookaheadPoint;
}

bool UAC_SplineMovementComponent::IsTryingToMove() const
{
    return m_LastRecordedSpeed > UE_SMALL_NUMBER;
}

