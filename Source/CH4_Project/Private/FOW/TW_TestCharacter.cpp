#include "FOW/TW_TestCharacter.h"

#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Component/TWTeamComponent.h"
#include "Core/TWPlayerState.h"
#include "FOW/TWVisionComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

ATW_TestCharacter::ATW_TestCharacter()
{
    bUseControllerRotationYaw = false;
    GetCharacterMovement()->bOrientRotationToMovement = true;

    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    CameraBoom->SetupAttachment(RootComponent);
    CameraBoom->TargetArmLength = 1200.f;
    CameraBoom->SetRelativeRotation(FRotator(-60.f, 0.f, 0.f));
    CameraBoom->bInheritYaw = false;

    TopDownCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("TopDownCamera"));
    TopDownCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
    
    TeamComp = CreateDefaultSubobject<UTWTeamComponent>(TEXT("TeamComponent"));
    if (TeamComp)
    {
        TeamComp->SetTeamID(0);
    }
    
    VisionComp = CreateDefaultSubobject<UTWVisionComponent>(TEXT("VisionComponent"));
    if (VisionComp)
    {
        VisionComp->VisionRadius = 800.0f;
    }
    
    // FogManager 에서 식별용으로 사용
    Tags.Add(TEXT("Unit"));
}

void ATW_TestCharacter::PossessedBy(AController* NewController)
{
    Super::PossessedBy(NewController);
    
    if (ATWPlayerState* PS = NewController->GetPlayerState<ATWPlayerState>())
    {
        UpdatePlayerSlot(PS->PlayerSlot);
    }
}

void ATW_TestCharacter::OnRep_PlayerState()
{
    Super::OnRep_PlayerState();
    
    if (ATWPlayerState* PS = GetPlayerState<ATWPlayerState>())
    {
        UpdatePlayerSlot(PS->PlayerSlot);
    }
}

void ATW_TestCharacter::UpdatePlayerSlot(int32 newPlayerSlot)
{
    if (TeamComp) 
    {
        TeamComp->SetTeamID(newPlayerSlot);
    }
}

void ATW_TestCharacter::BeginPlay()
{
    Super::BeginPlay();
    
    if (APlayerController* PC = Cast<APlayerController>(GetController()))
    {
        if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
        {
            Subsystem->AddMappingContext(DefaultMappingContext, 0);
        }
    }
}

void ATW_TestCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
    {
        EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ATW_TestCharacter::Move);
    }
}

void ATW_TestCharacter::Move(const FInputActionValue& Value)
{
    FVector2D MovementVector = Value.Get<FVector2D>();

    if (Controller != nullptr)
    {
        AddMovementInput(FVector::ForwardVector, MovementVector.Y);
        AddMovementInput(FVector::RightVector, MovementVector.X);
    }
}
