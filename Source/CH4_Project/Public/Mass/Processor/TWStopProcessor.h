#pragma once

#include "CoreMinimal.h"
#include "MassTranslator.h"
#include "TWStopProcessor.generated.h"


UCLASS()
class CH4_PROJECT_API UTWStopProcessor : public UMassTranslator
{
	GENERATED_BODY()
	
public:
	UTWStopProcessor();
protected:
	FMassEntityQuery EntityQuery;
	
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;
};
