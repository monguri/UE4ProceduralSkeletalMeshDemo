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
	UPROPERTY(EditAnywhere, Category = General)
	UPhysicsAsset* OverridePhysicsAsset = nullptr;

	/** Debug draw physics asset or not. */
	UPROPERTY(EditAnywhere, Category = General)
	bool bDebugDrawPhysicsAsset = nullptr;

	/** Root bone. It`s position is determined by average of all spheres. */
	UPROPERTY(EditAnywhere, Category = General)
	FBoneReference RootBone;

	/** Damping of verlet integration at component space. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (UIMin = "0", ClampMin = "0"), Category = Physics)
	float Damping = 0.1f;

	/** Stiffness of distance constraint between each spheres. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (UIMin = "0", ClampMin = "0"), Category = Physics)
	float Stiffness = 0.05f;

	/** Damping for linear velocity at world space. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (UIMin = "0", ClampMin = "0"), Category = Physics)
	float WorldDampingLocation = 0.8f;

	/** Gravity which is producted by mass already. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (PinHiddenByDefault), Category = Physics)
	FVector Gravity = FVector::ZeroVector;

	/** Enable collision with other object or not. */
	UPROPERTY(EditAnywhere, Category = Collision)
	bool bEnableCollision = false;

	/** Ratio of penetrate depth to radius. */
	UPROPERTY(EditAnywhere, Meta = (EditCondition = "bEnableCollision", UIMin = "0", UIMax = "1", ClampMin = "0", ClampMax = "1"), Category = Collision)
	float PenetrateDepthRadiusRatio = 0.5f;

protected:
	// FAnimNode_SkeletalControlBase interface
	virtual void OnInitializeAnimInstance(const struct FAnimInstanceProxy* InProxy, const class UAnimInstance* InAnimInstance) override;
	void InitializeBoneReferences(const FBoneContainer& RequiredBones);
	virtual bool NeedsOnInitializeAnimInstance() const override { return true; }
	virtual void UpdateInternal(const FAnimationUpdateContext& Context) override;
	virtual bool IsValidToEvaluate(const class USkeleton* Skeleton, const struct FBoneContainer& RequiredBones) override;
	virtual void EvaluateSkeletalControl_AnyThread(struct FComponentSpacePoseContext& Output, TArray<struct FBoneTransform>& OutBoneTransforms) override;
	// End of FAnimNode_SkeletalControlBase interface

private:
	struct FPhysicsAssetSphere
	{
		FCompactPoseBoneIndex BoneIndex = FCompactPoseBoneIndex(INDEX_NONE);
		float Radius = 0.0f;

		FVector WorkLocation = FVector::ZeroVector;
		FVector WorkPrevLocation = FVector::ZeroVector;
	};

	void InitSpheres(FComponentSpacePoseContext& Input);

	UPROPERTY(Transient)
	UPhysicsAsset* UsePhysicsAsset = nullptr;

	TArray<FPhysicsAssetSphere> Spheres;
	FTransform PreCSToWS = FTransform::Identity;
	float DeltaTime = 0.0f;
};

