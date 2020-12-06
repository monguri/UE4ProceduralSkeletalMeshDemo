#pragma once

#include "AnimGraphNode_SkeletalControlBase.h"
#include "AnimNode_DrawPhysicsAsset.h"
#include "AnimGraphNode_DrawPhysicsAsset.generated.h"

UCLASS(MinimalAPI, meta=(Keywords = "Debug Draw Physics Asset"))
class UAnimGraphNode_DrawPhysicsAsset : public UAnimGraphNode_SkeletalControlBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category=Settings)
	FAnimNode_DrawPhysicsAsset Node;

public:
	// UEdGraphNode interface
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FText GetTooltipText() const override;
	virtual void ValidateAnimNodeDuringCompilation(USkeleton* ForSkeleton, FCompilerResultsLog& MessageLog) override;
	// End of UEdGraphNode interface

protected:
	// UAnimGraphNode_SkeletalControlBase interface
	virtual FText GetControllerDescription() const override;
	virtual const FAnimNode_SkeletalControlBase* GetNode() const override { return &Node; }
	// End of UAnimGraphNode_SkeletalControlBase interface
};

