#include "Mass/Processor/TWCurrentStateProcessor.h"

#include "MassCommonTypes.h"
#include "MassCommonFragments.h"
#include "MassExecutionContext.h"
#include "Mass/Fragments/TWCommandFragment.h"


UTWCurrentStateProcessor::UTWCurrentStateProcessor()
	:EntityQuery(*this)
{
	ExecutionFlags = (int32)EProcessorExecutionFlags::Client;
	ExecutionOrder.ExecuteInGroup = UE::Mass::ProcessorGroupNames::UpdateWorldFromMass;
	bRequiresGameThreadExecution = true;
}

void UTWCurrentStateProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	
}

void UTWCurrentStateProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	
}

