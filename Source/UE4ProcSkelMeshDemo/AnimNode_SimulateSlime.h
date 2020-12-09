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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (UIMin = "0", ClampMin = "0"), Category = Settings)
	float Damping = 0.1f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (UIMin = "0", ClampMin = "0"), Category = Settings)
	float WorldDampingLocation = 0.8f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (UIMin = "0", ClampMin = "0"), Category = Settings)
	float WorldDampingRotation = 0.8f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (UIMin = "0", ClampMin = "0"), Category = Settings)
	float Stiffness = 0.05f;

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
		FName BoneName;
		FTransform Transform = FTransform::Identity;
		float Radius = 0.0f;

		FCompactPoseBoneIndex WorkBoneIndex = FCompactPoseBoneIndex(INDEX_NONE);
		FVector WorkSphereLocation = FVector::ZeroVector;
		FVector WorkPrevSphereLocation = FVector::ZeroVector;
	};

	void InitSpheres(FComponentSpacePoseContext& Input);

	UPROPERTY(Transient)
	UPhysicsAsset* UsePhysicsAsset = nullptr;

	TArray<FPhysicsAssetSphere> Spheres;
	FTransform PreCompTransform;
};

