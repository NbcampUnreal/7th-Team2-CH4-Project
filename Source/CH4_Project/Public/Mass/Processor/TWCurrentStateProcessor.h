#pragma once

#include "CoreMinimal.h"
#include "MassTranslator.h"
#include "StateTreeExecutionTypes.h"
#include "Mass/Fragments/TWCommandFragment.h"
#include "TWCurrentStateProcessor.generated.h"

struct FStateTreeExecutionContext;

UCLASS()
class UTWCurrentStateProcessor : public UMassTranslator
{
	GENERATED_BODY()
public:
	UTWCurrentStateProcessor();
	
protected:
	TStateTreeExternalDataHandle<FTWCommandTypeFragment>StateTypeHandle;
	TStateTreeExternalDataHandle<FTWCommandDataFragment>StateDataHandle;
	
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;
	virtual void Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
};
