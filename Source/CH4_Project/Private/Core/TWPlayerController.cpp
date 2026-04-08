#include "Core/TWPlayerController.h"
#include "Core/TWPlayerState.h"
#include "Building/TWTroopSpawnBuilding.h"
#include "Building/TWPopulationBuilding.h"
#include "Building/TWBlockingBuilding.h"

#include "EnhancedInputComponent.h"
#include "InputAction.h"
#include "EnhancedInputSubsystems.h"
#include "EngineUtils.h"

void ATWPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocalController())
	{
		return;
	}

	ULocalPlayer* LocalPlayer = GetLocalPlayer();
	if (!LocalPlayer)
	{
		return;
	}

	UEnhancedInputLocalPlayerSubsystem* Subsystem =
		LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
	if (!Subsystem)
	{
		return;
	}

	if (!IMC_Default)
	{
		return;
	}

	Subsystem->AddMappingContext(IMC_Default, 0);
}

void ATWPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(InputComponent);
	if (!EnhancedInput)
	{
		return;
	}

	if (IA_TestSpawnTroop)
	{
		EnhancedInput->BindAction(
			IA_TestSpawnTroop,
			ETriggerEvent::Started,
			this,
			&ATWPlayerController::HandleTestSpawnTroop
		);
	}
	
	if (IA_TestIncreasePopulation)
	{
		EnhancedInput->BindAction(
			IA_TestIncreasePopulation,
			ETriggerEvent::Started,
			this,
			&ATWPlayerController::HandleTestIncreasePopulation
		);
	}
	
	if (IA_TestDamageBlockingBuilding)
	{
		EnhancedInput->BindAction(
			IA_TestDamageBlockingBuilding,
			ETriggerEvent::Started,
			this,
			&ATWPlayerController::HandleTestDamageBlockingBuilding
		);
	}
}

void ATWPlayerController::HandleTestSpawnTroop(const FInputActionValue& Value)
{
	ServerTestSpawnTroop();
}

void ATWPlayerController::ServerTestSpawnTroop_Implementation()
{
	ATWPlayerState* TWPS = GetPlayerState<ATWPlayerState>();
	if (!TWPS)
	{
		return;
	}

	const int32 MyPlayerSlot = TWPS->PlayerSlot;

	for (TActorIterator<ATWTroopSpawnBuilding> It(GetWorld()); It; ++It)
	{
		ATWTroopSpawnBuilding* TroopBuilding = *It;
		if (!TroopBuilding)
		{
			continue;
		}

		if (TroopBuilding->OwnerPlayerSlot != MyPlayerSlot)
		{
			continue;
		}

		TroopBuilding->RequestEnqueueTroop();
		return;
	}
}

void ATWPlayerController::HandleTestIncreasePopulation(const FInputActionValue& Value)
{
	ServerTestIncreasePopulation();
}

void ATWPlayerController::ServerTestIncreasePopulation_Implementation()
{
	ATWPlayerState* TWPS = GetPlayerState<ATWPlayerState>();
	if (!TWPS)
	{
		return;
	}

	const int32 MyPlayerSlot = TWPS->PlayerSlot;

	for (TActorIterator<ATWPopulationBuilding> It(GetWorld()); It; ++It)
	{
		ATWPopulationBuilding* PopulationBuilding = *It;
		if (!PopulationBuilding)
		{
			continue;
		}

		if (PopulationBuilding->OwnerPlayerSlot != MyPlayerSlot)
		{
			continue;
		}

		PopulationBuilding->RequestEnqueuePopulation();
		return;
	}
}

void ATWPlayerController::HandleTestDamageBlockingBuilding(const FInputActionValue& Value)
{
	ServerTestDamageBlockingBuilding();
}

void ATWPlayerController::ServerTestDamageBlockingBuilding_Implementation()
{
	ATWPlayerState* TWPS = GetPlayerState<ATWPlayerState>();
	if (!TWPS)
	{
		return;
	}

	const int32 MyPlayerSlot = TWPS->PlayerSlot;

	for (TActorIterator<ATWBlockingBuilding> It(GetWorld()); It; ++It)
	{
		ATWBlockingBuilding* BlockingBuilding = *It;
		if (!BlockingBuilding)
		{
			continue;
		}

		if (BlockingBuilding->OwnerPlayerSlot == MyPlayerSlot)
		{
			continue;
		}

		BlockingBuilding->ApplyDamageToBuilding(10);
		return;
	}
}
