// Fill out your copyright notice in the Description page of Project Settings.


#include "UPEdEngine.h"

#include "ISourceControlModule.h"

void UUPEdEngine::Init(IEngineLoop* InEngineLoop)
{
	Super::Init(InEngineLoop);
	
	const ISourceControlModule& SourceControlModule = ISourceControlModule::Get();
	ISourceControlProvider& SourceControlProvider = SourceControlModule.GetProvider();
	// 순서가 중요합니다. 낮은 값이 계층 구조에서 하위에 위치합니다.
	// 즉, 상위 브랜치의 변경사항이 자동으로 하위로 병합됩니다.
	// (자동 병합은 적절히 구성된 CI 파이프라인이 필요합니다)
	// 이 패러다임에서 브랜치가 높을수록 더 안정적이며, 변경사항은 수동으로 상위로 승격됩니다.
	// 와일드카드 패턴을 지원합니다 (예: "origin/feature/*").
	// 내부적으로 패턴은 런타임에 `git branch --remotes --list <패턴>`을 통해 해석되므로,
	// 패턴에 매칭되는 새 리모트 브랜치는 코드 변경 없이 자동으로 감지됩니다.
	const TArray<FString> Branches {
		"origin/main",
		"origin/release",
		"origin/develop",
		"origin/feature/*",
		"origin/hotfix/*"};
	SourceControlProvider.RegisterStateBranches(Branches, TEXT("Content"));
}
