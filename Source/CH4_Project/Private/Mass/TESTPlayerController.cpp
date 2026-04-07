// Fill out your copyright notice in the Description page of Project Settings.


#include "Mass/TESTPlayerController.h"

#include "EnhancedInputSubsystems.h"
#include "MassActorSubsystem.h"
#include "MassEntitySubsystem.h"
#include "MassSignalSubsystem.h"
#include "Components/StateTreeComponent.h"
#include "Kismet/GameplayStatics.h"
#include "MassSpawner.h"
#include "MassExecutionContext.h"
#include "MassNavigationFragments.h"
#include "MassNavigationTypes.h"
#include "MassStateTreeTypes.h"
#include "MassStateTreeSubsystem.h"
#include "MassStateTreeFragments.h"
#include "NavigationSystem.h"
#include "Mass/Fragments/CommandFragment.h"

void ATESTPlayerController::BeginPlay()
{
	Super::BeginPlay();
}

void ATESTPlayerController::OnInputClick()
{
	FHitResult HitResult;
	FVector LastClickLocation;
	if (GetHitResultUnderCursor(ECC_Visibility, false, HitResult))
	{
		LastClickLocation = HitResult.Location;
		ServerMoveCommand(LastClickLocation);
	}
}

void ATESTPlayerController::ServerMoveCommand_Implementation(FVector LastClickLocation)
{
	UMassEntitySubsystem* EntitySubsystem = GetWorld()->GetSubsystem<UMassEntitySubsystem>();
	if (!EntitySubsystem) return;
	UE_LOG(LogMass, Warning, TEXT("MulticastMoveCommand_Implementation"));
	FVector NavLocation = FVector::ZeroVector;
	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());

	FNavLocation ProjectedLocation;
	if (NavSys && NavSys->ProjectPointToNavigation(LastClickLocation, ProjectedLocation, FVector(500.f, 500.f, 500.f)))
	{
		LastClickLocation = ProjectedLocation.Location;
	}
	else
	{
		return;
	}


	FMassEntityManager& EntityManager = EntitySubsystem->GetMutableEntityManager();

	FMassCommandBuffer CommandBuffer;

	CommandBuffer.PushCommand<FMassDeferredSetCommand>(
		[LastClickLocation](FMassEntityManager& InOutEntityManager)
		{
			FMassEntityQuery EntityQuery;
			EntityQuery.Initialize(InOutEntityManager.AsShared());
			EntityQuery.AddRequirement<FTWCommandTypeFragment>(EMassFragmentAccess::ReadWrite);
			EntityQuery.AddRequirement<FMassStateTreeInstanceFragment>(EMassFragmentAccess::ReadOnly);
			EntityQuery.AddSharedRequirement<FTWCommandDataFragment>(EMassFragmentAccess::ReadWrite);

			TArray<FMassEntityHandle> CollectedEntities;
			FMassExecutionContext Context = InOutEntityManager.CreateExecutionContext(0.f);

			EntityQuery.ForEachEntityChunk(Context, [&CollectedEntities](FMassExecutionContext& Context)
			{
				for (int32 i = 0; i < Context.GetNumEntities(); ++i)
				{
					CollectedEntities.Add(Context.GetEntity(i));
				}
			});

			UE_LOG(LogMass, Warning, TEXT("%d"), CollectedEntities.Num());

			for (const FMassEntityHandle& Entity : CollectedEntities)
			{
				if (!InOutEntityManager.IsEntityActive(Entity)) continue;

				if (FTWCommandTypeFragment* TypeFragment = InOutEntityManager.GetFragmentDataPtr<
					FTWCommandTypeFragment>(Entity))
				{
					TypeFragment->SetType(ETWCommandType::MoveToLocation);
				}
			}

			if (CollectedEntities.IsValidIndex(0) && InOutEntityManager.IsEntityActive(CollectedEntities[0]))
			{
				if (FTWCommandDataFragment* SharedData = InOutEntityManager.GetSharedFragmentDataPtr<
					FTWCommandDataFragment>(CollectedEntities[0]))
				{
					SharedData->SetLocation(LastClickLocation);
				}
			}

			if (UMassSignalSubsystem* SignalSubsystem = InOutEntityManager.GetWorld()->GetSubsystem<
				UMassSignalSubsystem>())
			{
				SignalSubsystem->SignalEntities(UE::Mass::Signals::StateTreeActivate, CollectedEntities);
			}
		});

	EntityManager.AppendCommands(TSharedPtr<FMassCommandBuffer>(&CommandBuffer, [](FMassCommandBuffer*)
	{
	}));
}
