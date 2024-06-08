// Copyright Epic Games, Inc. All Rights Reserved.

#include "Character/SplineTestCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Character/Components/SDZ_SplineMovementComponent.h"


//////////////////////////////////////////////////////////////////////////
// ASplineTestCharacter

ASplineTestCharacter::ASplineTestCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<USDZ_SplineMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	// Set size for collision capsule
	//GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
		
	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	//GetCharacterMovement()->RotationRate = FRotator(0.0f, 7200.0f, 0.0f); // ...at this rotation rate
	//GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f); // ...at this rotation rate

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	//GetCharacterMovement()->JumpZVelocity = 700.f;
	//GetCharacterMovement()->AirControl = 0.35f;
	//GetCharacterMovement()->MaxWalkSpeed = 600.f;
	//GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	//GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller
	CameraBoom->bInheritRoll = false;

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)
}

void ASplineTestCharacter::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 720.0f, 0.0f); // ...at this rotation rate

	//Add Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
}

void ASplineTestCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

//	SetActorRotation(GetFlightRotation());
}


//////////////////////////////////////////////////////////////////////////
// Input

void ASplineTestCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent)) {
		

		//Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ASplineTestCharacter::Move);

		//Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ASplineTestCharacter::Look);

		EnhancedInputComponent->BindAction(SwitchMode, ETriggerEvent::Triggered, this, &ASplineTestCharacter::SwapMode);


		EnhancedInputComponent->BindAction(UpBiasAction, ETriggerEvent::Triggered, this, &ASplineTestCharacter::UpBias);
		EnhancedInputComponent->BindAction(DownBiasAction, ETriggerEvent::Triggered, this, &ASplineTestCharacter::DownBias);

	}

}

void ASplineTestCharacter::Move(const FInputActionValue& Value)
{
	const FVector2D inputMovement = Value.Get<FVector2D>();
	const FRotator inputYaw(0.0f, Controller->GetControlRotation().Yaw, 0.0f);

	if (inputMovement.X != 0.0f)
	{
		const FVector direction = inputYaw.RotateVector(FVector::RightVector);
		AddMovementInput(direction, inputMovement.X);
	}

	if (inputMovement.Y != 0.0f)
	{
		const FVector direction = inputYaw.RotateVector(FVector::ForwardVector);
		AddMovementInput(direction, inputMovement.Y);
	}

}

void ASplineTestCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}


void ASplineTestCharacter::SwapMode()
{
	if (auto movement = Cast<USDZ_SplineMovementComponent>(GetCharacterMovement()))
	{
		movement->SetUseSpline(!movement->GetUseSpline());
	}
	//m_NormalWalk = !m_NormalWalk;
	//if (m_NormalWalk)
	//{
	//	GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Walking);
	//}
	//else
	//{
	//	GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Custom);
	//}
}
void ASplineTestCharacter::UpBias()
{
	if (auto movement = Cast<USDZ_SplineMovementComponent>(GetCharacterMovement()))
	{
		movement->MoveBias = FMath::Min(1.0f, movement->MoveBias + 0.5f);
	}
}

void ASplineTestCharacter::DownBias()
{
	if (auto movement = Cast<USDZ_SplineMovementComponent>(GetCharacterMovement()))
	{
		movement->MoveBias = FMath::Max(0.0f, movement->MoveBias - 0.5f);
	}

}

void ASplineTestCharacter::OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode)
{
	Super::OnMovementModeChanged(PrevMovementMode, PreviousCustomMode);
}