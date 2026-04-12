#pragma once

#include "CoreMinimal.h"
#include "MassTranslator.h"
#include "TWSearchProcessor.generated.h"

UCLASS()
class UTWSearchProcessor : public UMassTranslator
{
	GENERATED_BODY()
	
public:
	UTWSearchProcessor();
protected:
	FMassEntityQuery EntityQuery;
	
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;
};
