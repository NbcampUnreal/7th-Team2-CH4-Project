#include "Mass/Traits/FWStandTrait.h"
#include "MassEntityTemplateRegistry.h"
#include "Mass/Fragments/TWStandFragment.h"

void UFWStandTrait::BuildTemplate(
	FMassEntityTemplateBuildContext& BuildContext, 
	const UWorld& World) const
{
	BuildContext.AddFragment<FTWStandFragment>();
	// 이후 공격 관련 Tragment 추가해서 사용할 예정
}
