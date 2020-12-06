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

	Spheres.Empty();
	if (UsePhysicsAsset == nullptr)
	{
		return;
	}

	for (int32 i = 0; i < UsePhysicsAsset->SkeletalBodySetups.Num(); ++i)
	{
		// スケルタルメッシュ差し替えで、BoneNameに対応したBoneIndexが変わることは想定しない
		int32 BoneIndex = SkeletalMeshComp->GetBoneIndex(UsePhysicsAsset->SkeletalBodySetups[i]->BoneName);
		if (BoneIndex == INDEX_NONE)
		{
			continue;
		}

		if (UsePhysicsAsset->SkeletalBodySetups[i]->AggGeom.SphereElems.Num() == 0)
		{
			continue;
		}

		// 一本の骨ごとに一個のスフィアしか想定しない。また、スフィア以外は考えない。物理アセットのデータが動的に書き換わることも想定しない。
		const FKSphereElem& SphereElem = UsePhysicsAsset->SkeletalBodySetups[i]->AggGeom.SphereElems[0]; 
		Spheres.Emplace(BoneIndex, SphereElem.GetTransform(), SphereElem.Radius);
	}
}

bool FAnimNode_SimulateSlime::IsValidToEvaluate(const class USkeleton* Skeleton, const struct FBoneContainer& RequiredBones)
{
	return (UsePhysicsAsset != nullptr);
}

namespace
{
	// UPhysicsAssetEditorSkeletalMeshComponent::GetPrimitiveTransform()を参考にしている
	FTransform GetPrimitiveTransform(UBodySetup* SharedBodySetup, FTransform& BoneTM, int32 BodyIndex, EAggCollisionShape::Type PrimType, int32 PrimIndex, float Scale)
	{
		FVector Scale3D(Scale);

		if (PrimType == EAggCollisionShape::Sphere)
		{
			FTransform PrimTM = SharedBodySetup->AggGeom.SphereElems[PrimIndex].GetTransform();
			PrimTM.ScaleTranslation(Scale3D);
			return PrimTM * BoneTM;
		}
		else if (PrimType == EAggCollisionShape::Box)
		{
			FTransform PrimTM = SharedBodySetup->AggGeom.BoxElems[PrimIndex].GetTransform();
			PrimTM.ScaleTranslation(Scale3D);
			return PrimTM * BoneTM;
		}
		else if (PrimType == EAggCollisionShape::Sphyl)
		{
			FTransform PrimTM = SharedBodySetup->AggGeom.SphylElems[PrimIndex].GetTransform();
			PrimTM.ScaleTranslation(Scale3D);
			return PrimTM * BoneTM;
		}
		else if (PrimType == EAggCollisionShape::Convex)
		{
			FTransform PrimTM = SharedBodySetup->AggGeom.ConvexElems[PrimIndex].GetTransform();
			PrimTM.ScaleTranslation(Scale3D);
			return PrimTM * BoneTM;
		}
		else if (PrimType == EAggCollisionShape::TaperedCapsule)
		{
			FTransform PrimTM = SharedBodySetup->AggGeom.TaperedCapsuleElems[PrimIndex].GetTransform();
			PrimTM.ScaleTranslation(Scale3D);
			return PrimTM * BoneTM;
		}

		// Should never reach here
		check(0);
		return FTransform::Identity;
	}
}

void FAnimNode_SimulateSlime::EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms)
{
	const USkeletalMeshComponent* SkeletalMeshComp = Output.AnimInstanceProxy->GetSkelMeshComponent();
	UWorld* World = GEngine->GetWorldFromContextObject(SkeletalMeshComp, EGetWorldErrorMode::LogAndReturnNull);

	// UPhysicsAssetEditorSkeletalMeshComponent::GetPrimitiveColor()のElemSelectedColorの色をデバッガで値を調べた
	const FColor ElemSelectedColor = FColor(222, 163, 9);

	for (int32 i = 0; i < Spheres.Num(); ++i)
	{
		const FPhysicsAssetSphere& Sphere = Spheres[i];
		check(Sphere.BoneIndex != INDEX_NONE);

		FTransform BoneTM = SkeletalMeshComp->GetBoneTransform(Sphere.BoneIndex);
		float Scale = BoneTM.GetScale3D().GetAbsMax();
		FVector VectorScale(Scale);
		BoneTM.RemoveScaling();

		FTransform SphereTM = Sphere.Transform;
		SphereTM.ScaleTranslation(VectorScale);
		SphereTM *= BoneTM;

		float Radius = Sphere.Radius;

		// UPhysicsAssetEditorSkeletalMeshComponent::RenderAssetTools()を参考にしている
		if (bDebugDrawPhysicsAsset)
		{
			FFunctionGraphTask::CreateAndDispatchWhenReady(
				[World, SphereTM, Radius, Scale, ElemSelectedColor]() {
					bool bPersistent = false;
					float LifeTime = 0.0f;
					const FColor& ShapeColor = FColor::Blue;

					::DrawDebugSphere(World, SphereTM.GetLocation(), Radius * Scale, 16, ElemSelectedColor, bPersistent, LifeTime, ESceneDepthPriorityGroup::SDPG_Foreground);
				},
				TStatId(), nullptr, ENamedThreads::GameThread
			);
		}
	}
}

