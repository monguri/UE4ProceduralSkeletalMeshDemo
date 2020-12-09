#include "AnimNode_SimulateSlime.h"
#include "Animation/AnimInstanceProxy.h"
#include "Components/SkeletalMeshComponent.h"
#include "ReferenceSkeleton.h"
#include "DrawDebugHelpers.h"

void FAnimNode_SimulateSlime::OnInitializeAnimInstance(const FAnimInstanceProxy* InProxy, const UAnimInstance* InAnimInstance)
{
	// FAnimNode_RigidBody::OnInitializeAnimInstance()を参考にしている
	const USkeletalMeshComponent* SkeletalMeshComp = InAnimInstance->GetSkelMeshComponent();
	const USkeletalMesh* SkeletalMeshAsset = SkeletalMeshComp->SkeletalMesh;

	const FReferenceSkeleton& SkelMeshRefSkel = SkeletalMeshAsset->RefSkeleton;
	UsePhysicsAsset = OverridePhysicsAsset ? OverridePhysicsAsset : InAnimInstance->GetSkelMeshComponent()->GetPhysicsAsset();

	// 配列をリセットし、また初回Evaluateで作り直す
	Spheres.Empty();
}

bool FAnimNode_SimulateSlime::IsValidToEvaluate(const class USkeleton* Skeleton, const struct FBoneContainer& RequiredBones)
{
	return (UsePhysicsAsset != nullptr);
}

void FAnimNode_SimulateSlime::InitSpheres(FComponentSpacePoseContext& Input)
{
	check(UsePhysicsAsset != nullptr);

	const FBoneContainer& BoneContainer = Input.Pose.GetPose().GetBoneContainer();
	const USkeletalMeshComponent* SkeletalMeshComp = Input.AnimInstanceProxy->GetSkelMeshComponent();

	for (int32 i = 0; i < UsePhysicsAsset->SkeletalBodySetups.Num(); ++i)
	{
		if (UsePhysicsAsset->SkeletalBodySetups[i]->BoneName.IsNone())
		{
			continue;
		}

		if (UsePhysicsAsset->SkeletalBodySetups[i]->AggGeom.SphereElems.Num() == 0)
		{
			continue;
		}

		// 一本の骨ごとに一個のスフィアしか想定しない。また、スフィア以外は考えない。物理アセットのデータが動的に書き換わることも想定しない。
		const FKSphereElem& SphereElem = UsePhysicsAsset->SkeletalBodySetups[i]->AggGeom.SphereElems[0]; 

		FPhysicsAssetSphere Sphere;
		Sphere.BoneName = UsePhysicsAsset->SkeletalBodySetups[i]->BoneName;
		Sphere.Transform = SphereElem.GetTransform();
		Sphere.Radius = SphereElem.Radius;

		int32 BoneIndex = SkeletalMeshComp->GetBoneIndex(Sphere.BoneName);
		Sphere.WorkBoneIndex = BoneContainer.GetCompactPoseIndexFromSkeletonIndex(BoneIndex);

		FTransform BoneTM = Input.Pose.GetComponentSpaceTransform(Sphere.WorkBoneIndex);
		float Scale = BoneTM.GetScale3D().GetAbsMax();
		FVector VectorScale(Scale);
		BoneTM.RemoveScaling();

		FTransform SphereLocalTM = Sphere.Transform;
		SphereLocalTM.ScaleTranslation(VectorScale);

		FTransform SimulateTM = SphereLocalTM * BoneTM;

		Sphere.WorkSphereLocation = SimulateTM.GetLocation();
		Sphere.WorkPrevSphereLocation = SimulateTM.GetLocation();

		Spheres.Add(Sphere);
	}
}

void FAnimNode_SimulateSlime::EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms)
{
	const FBoneContainer& BoneContainer = Output.Pose.GetPose().GetBoneContainer();

	const USkeletalMeshComponent* SkeletalMeshComp = Output.AnimInstanceProxy->GetSkelMeshComponent();
	UWorld* World = GEngine->GetWorldFromContextObject(SkeletalMeshComp, EGetWorldErrorMode::LogAndReturnNull);

	if (Spheres.Num() == 0)
	{
		InitSpheres(Output);
	}

	// UPhysicsAssetEditorSkeletalMeshComponent::GetPrimitiveColor()のElemSelectedColorの色をデバッガで値を調べた
	const FColor ElemSelectedColor = FColor(222, 163, 9);

	// とりあえず相互作用のイテレーションは一回
	for (int32 i = 0; i < Spheres.Num(); ++i)
	{
		FPhysicsAssetSphere& Sphere = Spheres[i];
		check(!Sphere.BoneName.IsNone());

		// 動的にボーン名からFCompactPoseBoneIndexを取得する
		int32 BoneIndex = SkeletalMeshComp->GetBoneIndex(Sphere.BoneName);
		Sphere.WorkBoneIndex = BoneContainer.GetCompactPoseIndexFromSkeletonIndex(BoneIndex);

		FTransform BoneTM = Output.Pose.GetComponentSpaceTransform(Sphere.WorkBoneIndex);
		float Scale = BoneTM.GetScale3D().GetAbsMax();
		FVector VectorScale(Scale);
		BoneTM.RemoveScaling();

		FTransform SphereLocalTM = Sphere.Transform;
		SphereLocalTM.ScaleTranslation(VectorScale);

		FTransform SimulateTM = SphereLocalTM * BoneTM;

		float Radius = Sphere.Radius;

		// UPhysicsAssetEditorSkeletalMeshComponent::RenderAssetTools()を参考にしている
		if (bDebugDrawPhysicsAsset)
		{
			FFunctionGraphTask::CreateAndDispatchWhenReady(
				[World, SimulateTM, Radius, Scale, ElemSelectedColor]() {
					bool bPersistent = false;
					float LifeTime = 0.0f;
					const FColor& ShapeColor = FColor::Blue;

					::DrawDebugSphere(World, SimulateTM.GetLocation(), Radius * Scale, 16, ElemSelectedColor, bPersistent, LifeTime, ESceneDepthPriorityGroup::SDPG_Foreground);
				},
				TStatId(), nullptr, ENamedThreads::GameThread
			);
		}

		for (int32 j = 0; j < Spheres.Num(); ++j)
		{
			if (i == j)
			{
				continue;
			}

			FPhysicsAssetSphere& AnotherSphere = Spheres[j];

			int32 AnotherBoneIndex = SkeletalMeshComp->GetBoneIndex(Sphere.BoneName);
			AnotherSphere.WorkBoneIndex = BoneContainer.GetCompactPoseIndexFromSkeletonIndex(AnotherBoneIndex);

			FTransform AnotherBoneTM = Output.Pose.GetComponentSpaceTransform(AnotherSphere.WorkBoneIndex);
			float AnotherScale = BoneTM.GetScale3D().GetAbsMax();
			FVector AnotherVectorScale(AnotherScale);
			AnotherBoneTM.RemoveScaling();

			FTransform AnotherSphereLocalTM = AnotherSphere.Transform;
			AnotherSphereLocalTM.ScaleTranslation(AnotherVectorScale);

			const FTransform& AnotherSimulateTM = AnotherSphereLocalTM * AnotherBoneTM;

			float AnotherRadius = AnotherSphere.Radius;
		}

		Sphere.WorkSphereLocation = SimulateTM.GetLocation();
	}

	for (const FPhysicsAssetSphere& Sphere : Spheres)
	{
		OutBoneTransforms.Add(FBoneTransform(Sphere.WorkBoneIndex, FTransform(Sphere.WorkSphereLocation)));
	}

	OutBoneTransforms.Sort(FCompareBoneTransformIndex());
}

