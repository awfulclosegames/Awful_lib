// Copyright Strati D. Zerbinis 2025. All Rights Reserved.


#include "Character/Components/SDZ_SplineMovementComponent.h"
#include "MathUtil.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"
#include "VisualLogger/VisualLogger.h"

#include "Spline/SDZ_KBSpline.h"

#include "Character/SPlineTestCharacter.h"

//#pragma optimize("",off)

DEFINE_LOG_CATEGORY_STATIC(LogSplineMovement, Log, All);

static TAutoConsoleVariable<bool> CVarSDZ_SplineMoveDebug(TEXT("sdz.SplineMovement.Debug"), true, TEXT("Enable/Disable debug visualization for the Point of interest system"));


void USDZ_SplineMovementComponent::BeginPlay()
{
    Super::BeginPlay();
    m_Character = Cast<ASplineTestCharacter>(GetOwner());
    ensure(IsValid(m_Character));

    m_SplineConfig = USDZ_KBSpline::CreateSplineConfig(m_Character->GetActorLocation() - (m_Character->GetActorForwardVector() * GetMaxSpeed() * MovementResponse));
    //m_RotationResponseWeight = 1.0f / FMath::Max(0.0001f, OrientationResponse);

    ResetSplineState();
}

void USDZ_SplineMovementComponent::DecreaseResponse()
{
    MovementResponse *= 0.75f;

}

void USDZ_SplineMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    Speed = Velocity.Length();
}

void USDZ_SplineMovementComponent::IncreaseResponse()
{
    MovementResponse /= 0.75f;

}

void USDZ_SplineMovementComponent::ControlledCharacterMove(const FVector& InputVector, float DeltaSeconds)
{
    FVector input = InputVector.GetClampedToMaxSize(1.0f);
    const float RequestedSpeedSquared = input.SizeSquared();

    if (RequestedSpeedSquared > UE_KINDA_SMALL_NUMBER)
    {
        UpdateSplinePoints(DeltaSeconds, input);

        FVector RequestedTarget = input * GetMaxSpeed() * ControlLookahead;
        FVector MovementResponseTarget = input * GetMaxSpeed() * MovementResponse;

#if !UE_BUILD_SHIPPING
        if (CVarSDZ_SplineMoveDebug.GetValueOnAnyThread())
        {
            RequestedTarget.Z = 0.0f;
            MovementResponseTarget.Z = 0.0f;
            DrawDebugSphere(GetWorld(), m_Character->GetActorLocation() + RequestedTarget, 5.0f, 8, FColor::Black, false);
            DrawDebugSphere(GetWorld(), m_Character->GetActorLocation() + MovementResponseTarget, 5.0f, 8, FColor::Orange, false);

            if (m_SplineWalk)
            {
                USDZ_KBSpline::DrawDebug(m_Character, m_SplineConfig, m_SplineState, FColor::Blue, 1.0f);
            }
        }
#endif
    }
    else
    {
        ResetSplineState();
    }

    Super::ControlledCharacterMove(input, DeltaSeconds);
}

FRotator USDZ_SplineMovementComponent::ComputeOrientToMovementRotation(const FRotator& CurrentRotation, float DeltaTime, FRotator& DeltaRotation) const
{
    return Super::ComputeOrientToMovementRotation(CurrentRotation, DeltaTime, DeltaRotation);
}


void USDZ_SplineMovementComponent::UpdateSplinePoints(float DeltaT, const FVector& Input)
{
    FVector DrivenInput = Input;
    m_Throttle = DrivenInput.Length() * GetMaxSpeed();
    float targetTime = MovementResponse;
    DrivenInput.Z = 0.0f;
    m_SplineConfig->CommitPoint = 3;

    m_SplineConfig->ClearToCommitments();

    FVector nextPointTarget = m_SplineConfig->ControlPoints.Last().Location;
    nextPointTarget += (DrivenInput * GetMaxSpeed() * targetTime);
    nextPointTarget.Z = m_Character->GetActorLocation().Z;
    USDZ_KBSpline::AddSplinePoint(m_SplineConfig, { nextPointTarget , MoveTensioning, MoveBias });
}

// This method tries to follow the spline by sampling a point and moving towards it until it's too close, then sampling a new one by updating the 
//    chord time against the current character projected position or by adding a fixed 'quantum step'.
// an alternate and possibly better approach would by to use a critically damped spring to adjust the chord time update rate to self adjust in a way 
//    that converges on an ideal offset distance
void USDZ_SplineMovementComponent::EvaluateNavigationSpline(float DeltaT)
{
    FVector momentumDir = Velocity.GetSafeNormal();
    float expectedTravel = m_Throttle * DeltaT;
    FVector targetOffset = m_CurrentMoveTarget - m_Character->GetActorLocation();

    // if we're too close, then try and update the point on the spline that we are following
    if (targetOffset.Dot(momentumDir) < expectedTravel)
    {
        m_CurrentMoveTarget = m_Character->GetActorLocation();
        targetOffset = FVector::ZeroVector;

        // try and update the target point, potentially crossing up to 4 more segments
        for (int i = 0; i < 4; ++i)
        {
            if (m_SplineState.IsValidSegment())
            {
                // try and update the point within the segment
                float quantumUpdate = (DeltaT * 2.0f * Speed) / m_CurrentSegLen;

                float candidateTime = m_SegmentChordDir.Dot((m_Character->GetActorLocation() - m_SplineState.WorkingSet[FKBSplineState::FromPoint].Location) + (momentumDir * expectedTravel)) / m_CurrentSegLen;
                m_SplineState.Time = quantumUpdate + FMath::Max(m_SplineState.Time, candidateTime);

                FVector candidateTarget = USDZ_KBSpline::Sample(m_SplineState);
                FVector candidateOfset = candidateTarget - m_Character->GetActorLocation();

                if (m_SplineState.Time <= 1.0f && candidateOfset.Dot(momentumDir) > expectedTravel)
                {
                    m_CurrentMoveTarget = candidateTarget;
                    targetOffset = candidateOfset;
                    break;
                }
            }
            else
            {
                // try and get a new segment
                int proposedSegment = m_SplineConfig->GetNextCandidateSegment(m_SplineState.CurrentTraversalSegment);
  
                m_SplineState = USDZ_KBSpline::PrepareForEvaluation(m_SplineConfig, proposedSegment);

                USDZ_KBSpline::GetChord(m_SplineConfig, proposedSegment, m_SegmentChordDir);
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

#if !UE_BUILD_SHIPPING
    if (CVarSDZ_SplineMoveDebug.GetValueOnAnyThread())
    {
        DebugDrawEvaluateForVelocity(DeltaT);
    }
#endif
}



void USDZ_SplineMovementComponent::MoveAlongRail(const FVector& MomentumDir, FVector& TargetOffset, float DeltaT)
{
    // if we're not moving, don't bother
    if (TargetOffset.SquaredLength() > 0.0f)
    {
        // we're locking to the rail so clamp the movement 
        if (bForceStayOnRail)
        {
            if (DeltaT > 0.0f)
            {
                float offsetDist = TargetOffset.Length();
                float travelDist = FMath::Min(offsetDist / DeltaT, m_Throttle);

                Velocity = (TargetOffset / offsetDist) * travelDist;
            }
            return;
        }

        // Try and aim at the nearest point on the (potentially non-zero width) rail to try and travel to. 
        // Travel will be at the current set movement speed so the character may still overshoot or fall short of the rail
        FVector errorVec = (MomentumDir * MomentumDir.Dot(TargetOffset)) - TargetOffset;

#if !UE_BUILD_SHIPPING
        if (CVarSDZ_SplineMoveDebug.GetValueOnAnyThread())
        {
            FVector debug_RailWidthVec = errorVec.GetSafeNormal() * RailWidth * 0.5f;
            UE_VLOG_SEGMENT_THICK(GetOwner(), LogSplineMovement, Verbose, m_Character->GetActorLocation() + TargetOffset, m_Character->GetActorLocation() + TargetOffset + debug_RailWidthVec, FColor::Black, 5.0f, TEXT("Rail"));

            UE_VLOG_SEGMENT_THICK(GetOwner(), LogSplineMovement, Verbose, m_Character->GetActorLocation() + TargetOffset, m_Character->GetActorLocation(), FColor::Orange, 4.0f, TEXT("Goal"));
            UE_VLOG_SEGMENT_THICK(GetOwner(), LogSplineMovement, Verbose, m_Character->GetActorLocation() + TargetOffset, m_Character->GetActorLocation() + TargetOffset + errorVec, FColor::Red, 2.0f, TEXT(""));
        }
#endif

        // if the error is more than the width of the rail, then aim for the near edge of the rail
        if (errorVec.SquaredLength() > FMath::Square(RailWidth * 0.5f))
        {
            // can do the unsafe normalize since we know it's length is non-zero from the conditional
            errorVec = errorVec.GetUnsafeNormal() * RailWidth * 0.5f;
        }
        TargetOffset += errorVec;

        UE_VLOG_SEGMENT_THICK(GetOwner(), LogSplineMovement, Verbose, m_Character->GetActorLocation() + TargetOffset, m_Character->GetActorLocation(), FColor::Purple, 1.0f, TEXT("intention"));

        FVector oldVel = Velocity;
        Velocity = TargetOffset.GetSafeNormal() * m_Throttle;

        if (DeltaT > 0.0f)
        {
            Acceleration = (Velocity - oldVel) / DeltaT;
        }
    }
}



void USDZ_SplineMovementComponent::DebugDrawEvaluateForVelocity(float DeltaT)
{
#if !UE_BUILD_SHIPPING

    UE_VLOG(GetOwner(), LogSplineMovement, Verbose, TEXT("*****************************\n        Segment: %i\n        Commit: %i\n        m_CurrentMoveTarget: %s\n        targetOffset: %s\n         Current Time: %f\n*****************************"),
        m_SplineState.CurrentTraversalSegment, m_SplineConfig->CommitPoint,
        *m_CurrentMoveTarget.ToString(), *(m_CurrentMoveTarget - m_Character->GetActorLocation()).ToString(),
        m_SplineState.Time
    );


    UE_VLOG_LOCATION(GetOwner(), LogSplineMovement, Verbose, m_SplineState.WorkingSet[FKBSplineState::FromPoint].Location, 1.0f, FColor::Green, TEXT("From"));
    UE_VLOG_LOCATION(GetOwner(), LogSplineMovement, Verbose, m_SplineState.WorkingSet[FKBSplineState::ToPoint].Location, 1.0f, FColor::Red, TEXT("To"));

    UE_VLOG_LOCATION(GetOwner(), LogSplineMovement, Verbose, m_Character->GetActorLocation(), 1.0f, FColor::Blue, TEXT("Loc"));
    UE_VLOG_SEGMENT_THICK(GetOwner(), LogSplineMovement, Verbose, m_Character->GetActorLocation(), m_Character->GetActorLocation() + (Velocity * DeltaT), FColor::Black, 4.0f, TEXT(""));

    UE_VLOG_SEGMENT_THICK(GetOwner(), LogSplineMovement, Verbose, m_Character->GetActorLocation(), m_Character->GetActorLocation() + (Velocity * DeltaT), FColor::Purple, 3.0f, TEXT("Vel Step"));
    UE_VLOG_LOCATION(GetOwner(), LogSplineMovement, Verbose, m_Character->GetActorLocation() + (Velocity * DeltaT), 1.0f, FColor::Red, TEXT("Vel Step"));

#endif
}



void USDZ_SplineMovementComponent::ResetSplineState()
{
    USDZ_KBSpline::Reset(m_SplineConfig);

    USDZ_KBSpline::AddSplinePoint(m_SplineConfig, { m_Character->GetActorLocation() - (m_Character->GetActorForwardVector() * GetMaxSpeed() * MovementResponse) , MoveTensioning, MoveBias });
    USDZ_KBSpline::AddSplinePoint(m_SplineConfig, { m_Character->GetActorLocation()  , MoveTensioning, MoveBias });

    m_SplineState.CurrentTraversalSegment = 0;
    m_CurrentMoveTarget = m_Character->GetActorLocation();

    m_CurrentSegLen = 1.0f;
}

void USDZ_SplineMovementComponent::SetUseSpline(bool Value)
{
    bool prevValue = m_SplineWalk;
    m_SplineWalk = Value;
    if (m_SplineWalk && !prevValue)
    {
        ResetSplineState();
    }
}


FRotator operator*(const FRotator& A, const FRotator& B)
{
    return FRotator(A.Pitch * B.Pitch, A.Yaw * B.Yaw, A.Roll * B.Roll);
}

void USDZ_SplineMovementComponent::CalcVelocity(float DeltaTime, float Friction, bool bFluid, float BrakingDeceleration)
{
    if (m_SplineWalk)
    {
        EvaluateNavigationSpline(DeltaTime);
    }

    Super::CalcVelocity(DeltaTime, Friction, bFluid, BrakingDeceleration);
}
