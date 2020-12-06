#include "AnimGraphNode_SimulateSlime.h"
#include "Kismet2/CompilerResultsLog.h"
#include "AnimNode_DrawPhysicsAsset.h"

/////////////////////////////////////////////////////
// UAnimGraphNode_SimulateSlime

#define LOCTEXT_NAMESPACE "DrawPhysicsAsset"

FText UAnimGraphNode_SimulateSlime::GetControllerDescription() const
{
	return LOCTEXT("AnimGraphNode_SimulateSlime_ControllerDescription", "Simulate slime");
}

FText UAnimGraphNode_SimulateSlime::GetTooltipText() const
{
	return LOCTEXT("AnimGraphNode_SimulateSlime_Tooltip", "Simulate slime.");
}

FText UAnimGraphNode_SimulateSlime::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return FText(LOCTEXT("AnimGraphNode_SimulateSlime_NodeTitle", "SimulateSlime"));
}

void UAnimGraphNode_SimulateSlime::ValidateAnimNodeDuringCompilation(USkeleton* ForSkeleton, FCompilerResultsLog& MessageLog)
{
	Super::ValidateAnimNodeDuringCompilation(ForSkeleton, MessageLog);
}

#undef LOCTEXT_NAMESPACE
