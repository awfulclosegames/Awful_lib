// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Components/SDZ_SplineMovementComponent.h"
#include "MathUtil.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"

#include "Character/SPlineTestCharacter.h"
//#include "Character/Controlers/AV_PlayerController.h"
//#include "Character/AviothicCharacter.h"



DEFINE_LOG_CATEGORY_STATIC(LogSplineMovement, Log, All);



void USDZ_SplineMovementComponent::BeginPlay()
{
    Super::BeginPlay();
    m_Character = Cast<ASplineTestCharacter>(GetOwner());
    ensure(IsValid(m_Character));

}


void USDZ_SplineMovementComponent::SetMovementMode(EMovementMode NewMovementMode, uint8 NewCustomMode)
{
    bool IsAvFlight = NewMovementMode == EMovementMode::MOVE_Custom;

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

