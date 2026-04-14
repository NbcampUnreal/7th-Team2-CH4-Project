// Fill out your copyright notice in the Description page of Project Settings.


#include "Mass/TWUnit.h"


// Sets default values
ATWUnit::ATWUnit()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	SceneComponent = CreateDefaultSubobject<USceneComponent>("SceneComponent");
	SetRootComponent(SceneComponent);
	
	SkeletalMeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>("SkeletalMeshComponent");
	SkeletalMeshComponent->SetupAttachment(GetRootComponent());
	
	
	MassAgentComponent = CreateDefaultSubobject<UMassAgentComponent>("MassAgentComponent");
}

