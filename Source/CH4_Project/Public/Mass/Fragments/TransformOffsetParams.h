// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MassEntityElementTypes.h"
#include "TransformOffsetParams.generated.h"

/**
 * 
 */
USTRUCT()  
struct CH4_PROJECT_API FTransformOffsetParams : public FMassConstSharedFragment  
{  
	GENERATED_BODY()  
 
	/** Maximum time the smoothing can take. If it takes more than this values it will snap to the actual position */  
	UPROPERTY(EditAnywhere, Category = "", meta = (UIMin = 0.0f, ClampMin = 0.0f))  
	float MaxTimeToSmooth = 1.0f;  
 
	/** How much time the smooth can take */  
	UPROPERTY(EditAnywhere, Category = "", meta = (UIMin = 0.0f, ClampMin = 0.0f))  
	float SmoothTime = 0.9f;  
//	float SmoothTime = 0.2f;  
 
	/** The tolerated distance to smooth. If the distance is higher the mesh will snap to the actual position. */  
	UPROPERTY(EditAnywhere, Category = "", meta = (UIMin = 0.0f, ClampMin = 0.0f))  
	float MaxSmoothNetUpdateDistance = 500.0f;  
 
	float MaxSmoothNetUpdateDistanceSqr = 250000.0f;  
	
	UPROPERTY(EditAnywhere, Category = "", meta = (UIMin = 0.0f, ClampMin = 0.0f))  
	float MaxSmoothNetUpdateYaw = 180.0f;  
 
 
public:  
	/** Returns a copy of this instance with the parameters validated */
	FTransformOffsetParams GetValidated() const
	{  
		FTransformOffsetParams Params = *this;  
		Params.MaxTimeToSmooth = FMath::Max(0.0f, Params.MaxTimeToSmooth);  
		Params.SmoothTime = FMath::Max(0.0f, Params.SmoothTime);  
		Params.MaxSmoothNetUpdateDistance = FMath::Max(0.0f, Params.MaxSmoothNetUpdateDistance);  
		Params.MaxSmoothNetUpdateDistanceSqr = Params.MaxSmoothNetUpdateDistance * Params.MaxSmoothNetUpdateDistance;  
		return Params;  
	}
};