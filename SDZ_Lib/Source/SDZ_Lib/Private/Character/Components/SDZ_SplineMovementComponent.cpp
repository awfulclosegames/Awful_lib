// Copyright Strati D. Zerbinis 2025. All Rights Reserved.


#include "Character/Components/SDZ_SplineMovementComponent.h"
#include "MathUtil.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"

#include "Spline/SDZ_KBSpline.h"

#include "Character/SPlineTestCharacter.h"

DEFINE_LOG_CATEGORY_STATIC(LogSplineMovement, Log, All);

static TAutoConsoleVariable<bool> CVarSDZ_SplineMoveDebug(TEXT("sdz.SplineMovement.Debug"), true, TEXT("Enable/Disable debug visualization for the Point of interest system"));


void USDZ_SplineMovementComponent::BeginPlay()
{
    Super::BeginPlay();
    m_Character = Cast<ASplineTestCharacter>(GetOwner());
    ensure(IsValid(m_Character));

    m_SplineConfig = USDZ_KBSpline::CreateSplineConfig(m_Character->GetActorLocation() - (m_Character->GetActorForwardVector() * GetMaxSpeed() * m_HalfRespRate));
    m_HalfRespRate = MovementResponse * 0.5f;
    ResetSplineState();
}

void USDZ_SplineMovementComponent::DecreaseResponse()
{
    MovementResponse *= 0.75f;
    m_HalfRespRate = MovementResponse * 0.5f;

}
void USDZ_SplineMovementComponent::IncreaseResponse()
{
    MovementResponse /= 0.75f;

    m_HalfRespRate = MovementResponse * 0.5f;
}


void USDZ_SplineMovementComponent::PerformMovement(float DeltaSeconds)
{
    Super::PerformMovement(DeltaSeconds);
}

void USDZ_SplineMovementComponent::PhysWalking(float deltaTime, int32 Iterations)
{

    Super::PhysWalking(deltaTime, Iterations);
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

        if (m_SplineWalk)
        {
            EvaluateNavigationSpline(DeltaSeconds, input);
        }

#if !UE_BUILD_SHIPPING
        if (CVarSDZ_SplineMoveDebug.GetValueOnAnyThread())
        {
            RequestedTarget.Z = 0.0f;
            MovementResponseTarget.Z = 0.0f;
            DrawDebugSphere(GetWorld(), m_Character->GetActorLocation() + RequestedTarget, 5.0f, 8, FColor::Black, false);
            DrawDebugSphere(GetWorld(), m_Character->GetActorLocation() + MovementResponseTarget, 5.0f, 8, FColor::Orange, false);
            //if (m_SplineWalk && m_ValidSpline)
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


void USDZ_SplineMovementComponent::UpdateSplinePoints(float DeltaT, const FVector& Input)
{
    //float quantumUpdate = DeltaT / m_HalfRespRate;
    // project onto the spline segment chord, the character's expected movement. Clumsy! but functional for this demo
    //FVector expectedMovement = m_Character->GetActorForwardVector() * m_SegmentVelHeur * DeltaT;
    //m_currentSplineTime += FMath::Max(quantumUpdate, m_SegmentChordDir.Dot(expectedMovement)) / m_CurrentSegLen;

    FVector DrivenInput = Input;

    float targetTime = MovementResponse;
    DrivenInput.Z = 0.0f;
    m_SplineConfig->CommitPoint = 2;

    m_SplineConfig->ClearToCommitments();

    m_NextPointTarget = m_SplineConfig->ControlPoints.Last().Location;
    m_NextPointTarget += (DrivenInput * GetMaxSpeed() * targetTime);
    USDZ_KBSpline::AddSplinePoint(m_SplineConfig, { m_NextPointTarget , MoveTensioning, MoveBias });

    //FVector candidatePoint = m_Character->GetActorLocation() + (DrivenInput * GetMaxSpeed() * targetTime);
    //float newSegLeng = (candidatePoint - m_NextPointTarget).Length();
    //if (newSegLeng >= (GetMaxSpeed() * m_HalfRespRate))
    //{
    //    m_NextPointTarget = candidatePoint;
    //    USDZ_KBSpline::AddSplinePoint(m_SplineConfig, { m_NextPointTarget , MoveTensioning, MoveBias });
    //}






    //float targetTime = m_HalfRespRate;
    ////float targetTime = MovementResponse;

    ////m_ValidSpline = m_currentSplineTime < 1.0f && m_SegmentVelHeur > 0.0f;

    //DrivenInput.Z = 0.0f;
    ////m_NextPointTarget -= m_Character->GetActorLocation();

    //m_NextPointTarget = m_Character->GetActorLocation() + (DrivenInput * GetMaxSpeed() * targetTime);
    //USDZ_KBSpline::AddSplinePoint(m_SplineConfig, { m_NextPointTarget , MoveTensioning, MoveBias });
    //
    //m_NextPointTarget += (DrivenInput * GetMaxSpeed() * targetTime);
    //USDZ_KBSpline::AddSplinePoint(m_SplineConfig, { m_NextPointTarget , MoveTensioning, MoveBias });
}

void USDZ_SplineMovementComponent::UpdateSplineDirection(float DeltaT, FVector& outInput)
{
    float quantumUpdate = DeltaT / m_HalfRespRate;
    // project onto the spline segment chord, the character's expected movement. Clumsy! but functional for this demo
    FVector expectedMovement = m_Character->GetActorForwardVector() * m_SegmentVelHeur * DeltaT;
    m_currentSplineTime += FMath::Max(quantumUpdate, m_SegmentChordDir.Dot(expectedMovement)) / m_CurrentSegLen;

    FVector DrivenInput = outInput;

    float targetTime = m_HalfRespRate;
    m_ValidSpline = m_currentSplineTime < 1.0f && m_SegmentVelHeur > 0.0f;

    for (int i = 0; (i < 4) && !m_ValidSpline; ++i)
    {
        int proposedSegment = m_SplineConfig->GetNextCandidateSegment(m_SplineState.CurrentTraversalSegment);
        if (m_SplineConfig->IsValidSegment(proposedSegment))
        {
            m_SplineState = USDZ_KBSpline::PrepareForEvaluation(m_SplineConfig, proposedSegment);

            USDZ_KBSpline::GetChord(m_SplineConfig, proposedSegment, m_SegmentChordDir);
            m_CurrentSegLen = m_SegmentChordDir.Length();
            if (m_CurrentSegLen > 0.0f)
            {
                m_SegmentChordDir /= m_CurrentSegLen;
            }

            m_SegmentVelHeur = m_CurrentSegLen / targetTime;

            expectedMovement = m_Character->GetActorForwardVector() * m_SegmentVelHeur * DeltaT;

            m_currentSplineTime = FMath::Max(quantumUpdate, m_SegmentChordDir.Dot(expectedMovement)) / m_CurrentSegLen;
            m_ValidSpline = m_currentSplineTime < 1.0f && m_SegmentVelHeur > 0.0f;
          
            if (m_ValidSpline)
            {
                // we can consume our last segment
                m_SplineConfig->ConsumeSegment(m_LastValidSegment);
            }
        }
        else
        {
            DrivenInput.Z = 0.0f;
            m_NextPointTarget += DrivenInput * GetMaxSpeed() * targetTime;

            USDZ_KBSpline::AddSplinePoint(m_SplineConfig, { m_NextPointTarget , MoveTensioning, MoveBias });
        }
    }

    if (m_ValidSpline)
    {
        m_LastValidSegment = m_SplineState.CurrentTraversalSegment;
        FVector OriginalInput = outInput;
        m_SplineState.Time = m_currentSplineTime;
        FVector splinePoint = USDZ_KBSpline::Sample(m_SplineState);
        outInput = (splinePoint - m_Character->GetActorLocation()) / (GetMaxSpeed() * DeltaT);
    }
}

void USDZ_SplineMovementComponent::EvaluateNavigationSpline(float DeltaT, FVector& outInput)
{
    float quantumUpdate = DeltaT / m_HalfRespRate;
    // project onto the spline segment chord, the character's expected movement. Clumsy! but functional for this demo
    FVector expectedMovement = m_Character->GetActorForwardVector() * m_SegmentVelHeur * DeltaT;
    m_currentSplineTime += FMath::Max(quantumUpdate, m_SegmentChordDir.Dot(expectedMovement)) / m_CurrentSegLen;

    FVector DrivenInput = outInput;

    float targetTime = m_HalfRespRate;
    m_ValidSpline = m_currentSplineTime < 1.0f && m_SegmentVelHeur > 0.0f;

    for (int i = 0; (i < 4) && !m_ValidSpline; ++i)
    {
        int proposedSegment = m_SplineConfig->GetNextCandidateSegment(m_SplineState.CurrentTraversalSegment);
        if (m_SplineConfig->IsValidSegment(proposedSegment))
        {
            m_SplineState = USDZ_KBSpline::PrepareForEvaluation(m_SplineConfig, proposedSegment);

            USDZ_KBSpline::GetChord(m_SplineConfig, proposedSegment, m_SegmentChordDir);
            m_CurrentSegLen = m_SegmentChordDir.Length();
            if (m_CurrentSegLen > 0.0f)
            {
                m_SegmentChordDir /= m_CurrentSegLen;
            }

            m_SegmentVelHeur = m_CurrentSegLen / targetTime;

            expectedMovement = m_Character->GetActorForwardVector() * m_SegmentVelHeur * DeltaT;

            m_currentSplineTime = FMath::Max(quantumUpdate, m_SegmentChordDir.Dot(expectedMovement)) / m_CurrentSegLen;
            m_ValidSpline = m_currentSplineTime < 1.0f && m_SegmentVelHeur > 0.0f;

            if (m_ValidSpline)
            {
                // we can consume our last segment
                m_SplineConfig->ConsumeSegment(m_LastValidSegment);
            }
        }
    }

    if (m_ValidSpline)
    {
        m_LastValidSegment = m_SplineState.CurrentTraversalSegment;
        FVector OriginalInput = outInput;
        m_SplineState.Time = m_currentSplineTime;
        FVector splinePoint = USDZ_KBSpline::Sample(m_SplineState);
        outInput = (splinePoint - m_Character->GetActorLocation()) / (GetMaxSpeed() * DeltaT);
    }
}



void USDZ_SplineMovementComponent::ResetSplineState()
{
    USDZ_KBSpline::Reset(m_SplineConfig);

    USDZ_KBSpline::AddSplinePoint(m_SplineConfig, { m_Character->GetActorLocation() - (m_Character->GetActorForwardVector() * GetMaxSpeed() * m_HalfRespRate) , MoveTensioning, MoveBias });
    USDZ_KBSpline::AddSplinePoint(m_SplineConfig, { m_Character->GetActorLocation()  , MoveTensioning, MoveBias });

    m_SplineState.CurrentTraversalSegment = 0;
    m_currentSplineTime = 0.0f;
    m_SegmentVelHeur = 0.0f;
    m_CurrentSegLen = 1.0f;
    m_NextPointTarget = m_Character->GetActorLocation();
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

void USDZ_SplineMovementComponent::SetMovementMode(EMovementMode NewMovementMode, uint8 NewCustomMode)
{
    Super::SetMovementMode(NewMovementMode, NewCustomMode);
    bOrientRotationToMovement = true;
}

FRotator operator*(const FRotator& A, const FRotator& B)
{
    return FRotator(A.Pitch * B.Pitch, A.Yaw * B.Yaw, A.Roll * B.Roll);
}

void USDZ_SplineMovementComponent::PhysCustom(float deltaTime, int32 Iterations)
{
	Super::PhysCustom(deltaTime, Iterations);

}



void USDZ_SplineMovementComponent::HandleHitSomethign(const FVector& adjustedVel, FVector& location, FHitResult& hit, float deltaTime)
{
    if (hit.Time < 1.f)
    {
        const FVector gravDir = FVector(0.f, 0.f, -1.f);
        const FVector velDir = Velocity.GetSafeNormal();
        const float verticality = gravDir.Dot(velDir);

        bool bSteppedUp = false;
        if ((FMath::Abs(hit.ImpactNormal.Z) < 0.2f) && (verticality < 0.5f) && (verticality > -0.2f) && CanStepUp(hit))
        {
            float stepZ = UpdatedComponent->GetComponentLocation().Z;
            bSteppedUp = StepUp(gravDir, adjustedVel * (1.0f - hit.Time), hit);
            if (bSteppedUp)
            {
                location.Z = UpdatedComponent->GetComponentLocation().Z + (location.Z - stepZ);
            }
        }

        if (!bSteppedUp)
        {
            const FVector ReducedAdjusted = adjustedVel * 0.96f;
            //adjust and try again
            HandleImpact(hit, deltaTime, ReducedAdjusted);
            SlideAlongSurface(ReducedAdjusted, (1.0f - hit.Time), hit.Normal, hit, true);
        }
    }
}

