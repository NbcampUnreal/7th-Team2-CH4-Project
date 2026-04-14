#include "Mass/Traits/FWStandTrait.h"
#include "MassEntityTemplateRegistry.h"
#include "Mass/Fragments/TWCommandFragment.h"
#include "Mass/Fragments/TWStandFragment.h"
#include "Mass/Fragments/TWStatusFragment.h"
#include "Mass/Fragments/TWUnitFragment.h"

void UFWStandTrait::BuildTemplate(
	FMassEntityTemplateBuildContext& BuildContext, 
	const UWorld& World) const
{
	BuildContext.AddTag<FFWStopMovementTag>();
	
	BuildContext.AddFragment<FTWStandFragment>();
	BuildContext.AddSharedFragment(FSharedStruct::Make<FTWCommandDataFragment>());
	BuildContext.AddFragment<FTWCommandTypeFragment>();
	BuildContext.AddFragment<FTWStatusFragment>();
	BuildContext.AddFragment<FTWUnitFragment>();
}
