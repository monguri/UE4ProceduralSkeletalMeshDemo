#include "AnimGraphNode_DrawPhysicsAsset.h"
#include "Kismet2/CompilerResultsLog.h"
#include "AnimNode_DrawPhysicsAsset.h"

/////////////////////////////////////////////////////
// UAnimGraphNode_DrawPhysicsAsset

#define LOCTEXT_NAMESPACE "DrawPhysicsAsset"

FText UAnimGraphNode_DrawPhysicsAsset::GetControllerDescription() const
{
	return LOCTEXT("AnimGraphNode_DrawPhysicsAsset_ControllerDescription", "Debug draw physics asset");
}

FText UAnimGraphNode_DrawPhysicsAsset::GetTooltipText() const
{
	return LOCTEXT("AnimGraphNode_DrawPhysicsAsset_Tooltip", "This debug draw physics asset.");
}

FText UAnimGraphNode_DrawPhysicsAsset::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return FText(LOCTEXT("AnimGraphNode_DrawPhysicsAsset_NodeTitle", "DrawPhysicsAsset"));
}

void UAnimGraphNode_DrawPhysicsAsset::ValidateAnimNodeDuringCompilation(USkeleton* ForSkeleton, FCompilerResultsLog& MessageLog)
{
	Super::ValidateAnimNodeDuringCompilation(ForSkeleton, MessageLog);
}

#undef LOCTEXT_NAMESPACE
