#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "BoneControllers/AnimNode_SkeletalControlBase.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "AnimNode_SimulateSlime.generated.h"

/**
 *	Debug draw physics asset.
 */
USTRUCT()
struct UE4PROCSKELMESHDEMO_API FAnimNode_SimulateSlime : public FAnimNode_SkeletalControlBase
{
	GENERATED_BODY()

public:
	/** Physics asset to use. If empty use the skeletal mesh's default physics asset */
	UPROPERTY(EditAnywhere, Category = Settings)
	UPhysicsAsset* OverridePhysicsAsset = nullptr;

	/** Debug draw physics asset or not. */
	UPROPERTY(EditAnywhere, Category = Settings)
	bool bDebugDrawPhysicsAsset = nullptr;

protected:
	// FAnimNode_SkeletalControlBase interface
	virtual void OnInitializeAnimInstance(const struct FAnimInstanceProxy* InProxy, const class UAnimInstance* InAnimInstance) override;
	virtual bool NeedsOnInitializeAnimInstance() const override { return true; }
	virtual bool IsValidToEvaluate(const class USkeleton* Skeleton, const struct FBoneContainer& RequiredBones) override;
	virtual void EvaluateSkeletalControl_AnyThread(struct FComponentSpacePoseContext& Output, TArray<struct FBoneTransform>& OutBoneTransforms) override;
	// End of FAnimNode_SkeletalControlBase interface

private:
	struct FPhysicsAssetSphere
	{
		int32 BoneIndex = INDEX_NONE;
		FTransform Transform = FTransform::Identity;
		float Radius;
		FPhysicsAssetSphere(int32 _BoneIndex, const FTransform& _Transform, float _Radius) : BoneIndex(_BoneIndex), Transform(_Transform), Radius(_Radius) {}
	};

	UPROPERTY(Transient)
	UPhysicsAsset* UsePhysicsAsset = nullptr;

	TArray<FPhysicsAssetSphere> Spheres;
};

