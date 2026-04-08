#include "Gameplay/Test/TWTestUnitActor.h"

#include "AIController.h"
#include "Components/CapsuleComponent.h"

ATWTestUnitActor::ATWTestUnitActor()
{
	PrimaryActorTick.bCanEverTick = false;

	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	GetCapsuleComponent()->SetCollisionResponseToAllChannels(ECR_Ignore);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);

	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
}

FName ATWTestUnitActor::GetUIEntityId() const
{
	return UnitId;
}

FString ATWTestUnitActor::GetUIDisplayName() const
{
	return DisplayName;
}

FString ATWTestUnitActor::GetUITypeLabel() const
{
	return TypeLabel;
}

float ATWTestUnitActor::GetCurrentHP() const
{
	return CurrentHPValue;
}

float ATWTestUnitActor::GetMaxHP() const
{
	return MaxHPValue;
}

TArray<FName> ATWTestUnitActor::GetAvailableCommandIds() const
{
	return AvailableCommandIds;
}

void ATWTestUnitActor::MoveToWorldLocation(const FVector& InLocation)
{
	if (AAIController* AIController = Cast<AAIController>(GetController()))
	{
		AIController->MoveToLocation(InLocation);
	}
}

void ATWTestUnitActor::StopUnit()
{
	if (AAIController* AIController = Cast<AAIController>(GetController()))
	{
		AIController->StopMovement();
	}
}

void ATWTestUnitActor::HoldPosition()
{
	if (AAIController* AIController = Cast<AAIController>(GetController()))
	{
		AIController->StopMovement();
	}
}

void ATWTestUnitActor::AttackTarget(AActor* InTarget)
{
	if (!InTarget)
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[TWTestUnitActor] AttackTarget called: %s -> %s"),
		*GetName(),
		*InTarget->GetName());
}