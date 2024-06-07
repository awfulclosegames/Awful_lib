// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Components/SDZ_SplineMovementComponent.h"
#include "MathUtil.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"

#include "Spline/SDZ_KBSpline.h"

#include "Character/SPlineTestCharacter.h"
//#include "Character/Controlers/AV_PlayerController.h"
//#include "Character/AviothicCharacter.h"

#pragma optimize("",off)

DEFINE_LOG_CATEGORY_STATIC(LogSplineMovement, Log, All);

static TAutoConsoleVariable<bool> CVarSDZ_SplineMoveDebug(TEXT("sdz.SplineMovement.Debug"), false, TEXT("Enable/Disable debug visualization for the Point of interest system"));


void USDZ_SplineMovementComponent::BeginPlay()
{
    Super::BeginPlay();
    m_Character = Cast<ASplineTestCharacter>(GetOwner());
    ensure(IsValid(m_Character));

    m_SplineConfig = USDZ_KBSpline::CreateSplineConfig(m_Character->GetActorLocation() - (m_Character->GetActorForwardVector() * GetMaxSpeed() * MovementResponse));
    
    USDZ_KBSpline::AddSplinePoint(m_SplineConfig, { m_Character->GetActorLocation() });
    //USDZ_KBSpline::AddSplinePoint(m_SplineConfig, { m_Character->GetActorLocation() + (m_Character->GetActorForwardVector() * GetMaxSpeed() * MovementResponse) });
    m_SplineState.CurrentTraversalSegment = 0;
    m_currentSplineTime = MovementResponse;

    m_MoveTarget = m_Character->GetActorLocation();
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
        FVector RequestedTarget = input * GetMaxSpeed() * ControlLookahead;
        FVector MovementResponseTarget = input * GetMaxSpeed() * MovementResponse;

        if (m_SplineWalk)
        {
            //m_MoveTarget += input * 4.0f * GetMaxSpeed() * DeltaSeconds;
            //FVector DeltaTarget = m_MoveTarget - m_Character->GetActorLocation();
            //float sqrLength = DeltaTarget.SquaredLength();
            //if (sqrLength > FMath::Square(GetMaxSpeed() * ControlLookahead))
            //{
            //    DeltaTarget = DeltaTarget / FMath::Sqrt(sqrLength);
            //    DeltaTarget *= GetMaxSpeed() * ControlLookahead;
            //    m_MoveTarget = m_Character->GetActorLocation() + DeltaTarget;
            //}

            //FVector DrivenInput = (DeltaTarget / (GetMaxSpeed() * ControlLookahead)).GetClampedToMaxSize(1.0f);
            //UE_LOG(LogSplineMovement, Log, TEXT(">>           INPUT: %f  DrivenInput : %f   "), input.Length(), DrivenInput.Length());

            //FVector AdjustedRequestedTarget = DrivenInput * GetMaxSpeed() * ControlLookahead;
            //MovementResponseTarget = DrivenInput * GetMaxSpeed() * MovementResponse;
            //DrawDebugSphere(GetWorld(), m_Character->GetActorLocation() + AdjustedRequestedTarget, 10.0f, 8, FColor::Blue, false);

            UpdateSplineDirection(DeltaSeconds, input);
            //input = DrivenInput;
        }

#if !UE_BUILD_SHIPPING
        if (CVarSDZ_SplineMoveDebug.GetValueOnAnyThread())
        {
            RequestedTarget.Z = 0.0f;
            MovementResponseTarget.Z = 0.0f;
            DrawDebugSphere(GetWorld(), m_Character->GetActorLocation() + RequestedTarget, 5.0f, 8, FColor::Black, false);
            DrawDebugSphere(GetWorld(), m_Character->GetActorLocation() + MovementResponseTarget, 5.0f, 8, FColor::Orange, false);
            if (m_SplineWalk && m_ValidSpline)
            {
                USDZ_KBSpline::DrawDebug(m_Character, m_SplineConfig, m_SplineState, FColor::Blue, 1.0f);
            }

        }
#endif
    }
    //else
    //{
    //    m_currentSplineTime = MovementResponse;
    //    m_ValidSpline = false;

    //    USDZ_KBSpline::Reset(m_SplineConfig);
    //    USDZ_KBSpline::AddSplinePoint(m_SplineConfig, { m_Character->GetActorLocation() - (m_Character->GetActorForwardVector() * GetMaxSpeed() * MovementResponse) });
    //    USDZ_KBSpline::AddSplinePoint(m_SplineConfig, { m_Character->GetActorLocation() });
    //}


    Super::ControlledCharacterMove(input, DeltaSeconds);
}


void USDZ_SplineMovementComponent::UpdateSplineDirection(float DeltaT, FVector& outInput)
{
    //m_MoveTarget += outInput * 3.0f * GetMaxSpeed() * DeltaT;
    //FVector DeltaTarget = m_MoveTarget - m_Character->GetActorLocation();
    //float sqrLength = DeltaTarget.SquaredLength();
    //if (sqrLength > FMath::Square(GetMaxSpeed() * ControlLookahead))
    //{
    //    DeltaTarget = DeltaTarget / FMath::Sqrt(sqrLength);
    //    DeltaTarget *= GetMaxSpeed() * ControlLookahead;
    //    m_MoveTarget = m_Character->GetActorLocation() + DeltaTarget;
    //}

    FVector DrivenInput = outInput;
    //FVector DrivenInput = (DeltaTarget / (GetMaxSpeed() * ControlLookahead)).GetClampedToMaxSize(1.0f);

    float targetTime = MovementResponse ;
    float remainingSplineTime = m_currentSplineTime / targetTime;
    m_ValidSpline = remainingSplineTime < 1.0f;
    for (int i = 0; (i < 4) && !m_ValidSpline; ++i)
    {
        //if (remainingSplineTime > 1.0f)
        {
            //m_ValidSpline = false;
            UE_LOG(LogSplineMovement, Log, TEXT(">>           Time > 1  remaining : %f   "), remainingSplineTime);

            int proposedSegment = m_SplineState.CurrentTraversalSegment + 1;
            if (m_SplineConfig->IsValidSegment(proposedSegment))
            {
                remainingSplineTime = FMath::Max(0.0f, remainingSplineTime - 1.0f);

                m_currentSplineTime = remainingSplineTime;

                m_SplineState = USDZ_KBSpline::PrepareForEvaluation(m_SplineConfig, proposedSegment);
                m_ValidSpline = remainingSplineTime < 1.0f;
            }
            else
            {
                UE_LOG(LogSplineMovement, Log, TEXT(">>           Adding POINT  OFFSET : I: %f  S: %f  T: %f (%f) "), DrivenInput , GetMaxSpeed() , targetTime, (DrivenInput * GetMaxSpeed() * targetTime));
                DrivenInput.Z = 0.0f;
                m_MoveTarget += DrivenInput * GetMaxSpeed() * targetTime;

                USDZ_KBSpline::AddSplinePoint(m_SplineConfig, { m_MoveTarget , -0.5f, 0.5f});
                DrawDebugLine(GetWorld(), m_Character->GetActorLocation(), m_MoveTarget, FColor::Orange, false, 2.0f);
  
                DrawDebugSphere(GetWorld(), m_MoveTarget, 10.0f, 8, FColor::Orange, false);
                DrawDebugSphere(GetWorld(), m_MoveTarget- DrivenInput * GetMaxSpeed() * targetTime, 10.0f, 8, FColor::Purple, false);

            }
        }
    }
    if (m_ValidSpline)
    {
        FVector OriginalInput = outInput;
        m_SplineState.Time = remainingSplineTime;
        outInput = (USDZ_KBSpline::Sample(m_SplineState) - m_Character->GetActorLocation()) / (GetMaxSpeed() * DeltaT);

        UE_LOG(LogSplineMovement, Log, TEXT(">>    Sample Time : %f      Deflection: %s   (%f :: D: %f  I: %f)   Updating: %f"),
            remainingSplineTime, *outInput.ToString(), 
            outInput.Length(), DrivenInput.Length(), OriginalInput.Length(), 
            m_currentSplineTime + DeltaT);
        UE_LOG(LogSplineMovement, Log, TEXT("***********************************************************"));
    }
    m_currentSplineTime += DeltaT;
}


void USDZ_SplineMovementComponent::SetUseSpline(bool Value)
{
    bool prevValue = m_SplineWalk;
    m_SplineWalk = Value;
    if (m_SplineWalk && !prevValue)
    {
        m_currentSplineTime = MovementResponse;
    }
}

void USDZ_SplineMovementComponent::SetMovementMode(EMovementMode NewMovementMode, uint8 NewCustomMode)
{
    //bool IsAvFlight = NewMovementMode == EMovementMode::MOVE_Custom;

    Super::SetMovementMode(NewMovementMode, NewCustomMode);
    bOrientRotationToMovement = true;

    //if (auto controller = Cast<AAV_PlayerController>(CharacterOwner->GetController()))
    //{

    //}

    if (IsValid(m_Character))
    {
        //m_Character->SetAVFlightBehaviour(IsAvFlight);
    }


}

FRotator operator*(const FRotator& A, const FRotator& B)
{
    return FRotator(A.Pitch * B.Pitch, A.Yaw * B.Yaw, A.Roll * B.Roll);
}

void USDZ_SplineMovementComponent::PhysCustom(float deltaTime, int32 Iterations)
{
	Super::PhysCustom(deltaTime, Iterations);

    if (deltaTime < MIN_TICK_TIME)
    {
        return;
    }
    bool validCharacter = CharacterOwner && (CharacterOwner->Controller || bRunPhysicsWithNoController || HasAnimRootMotion() || CurrentRootMotion.HasOverrideVelocity() || (CharacterOwner->GetLocalRole() == ROLE_SimulatedProxy));
    if (!validCharacter)
    {
        return;
    }

    
    auto controlRotation = CharacterOwner->Controller->GetControlRotation();

    bJustTeleported = false;
    float remainingTime = deltaTime;
    const FVector gravity(0.f, 0.f, GetGravityZ());
    FVector angularAccel(0.0f);

    RestorePreAdditiveRootMotionVelocity();

    // Update movement state
    while ((remainingTime >= MIN_TICK_TIME) && (Iterations < MaxSimulationIterations))
    {

        Iterations++;
        bJustTeleported = false;
        const float timeTick = GetSimulationTimeStep(remainingTime, Iterations);
        remainingTime -= timeTick;

        if (!HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity())
        {
            const float prevVerticalVel = Velocity.Z;

            CalcVelocity(timeTick, 0, false, 0);
        }


        FVector velDir = Velocity;
        float speed = Velocity.SquaredLength();
        if (speed > 0.0f)
        {
            speed = FMath::Sqrt(speed);
            velDir /= speed;
        }
        FVector localVelDir = CharacterOwner->GetActorQuat().UnrotateVector(velDir);



        // Apply gravity 
        Velocity = NewFallVelocity(Velocity, gravity, timeTick);


        ApplyRootMotionToVelocity(timeTick);

        FVector oldLocation = UpdatedComponent->GetComponentLocation();

        const FVector adjusted = Velocity * timeTick;
        FHitResult hit(1.f);
        SafeMoveUpdatedComponent(adjusted, UpdatedComponent->GetComponentQuat(), true, hit);

        HandleHitSomethign(adjusted, oldLocation, hit, timeTick);

        if (!bJustTeleported && !HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity())
        {
            Velocity = (UpdatedComponent->GetComponentLocation() - oldLocation) / timeTick;
        }
        
    }


    //m_Character->AddActorLocalRotation(inducedQRot);
    
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

